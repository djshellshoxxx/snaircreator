#include <JuceHeader.h>
#include "Parameters.h"
#include "SourceAnalyzer.h"
#include "SnairEngine.h"
#include "SessionStore.h"
#include "WavExporter.h"

namespace
{
SourceAudio makeSource(bool noisy=false)
{
    SourceAudio s;
    s.sampleRate=48000.0;s.channelCount=1;s.frameCount=48000;s.samples.setSize(1,(int)s.frameCount);s.samples.clear();
    for(int i=0;i<s.samples.getNumSamples();++i)
    {
        const float t=(float)i/48000.0f;
        float x=0.35f*std::sin(juce::MathConstants<float>::twoPi*180.0f*t)*std::exp(-t*6.0f);
        if(i>=12000 && i<12300) x += (1.0f-(i-12000)/300.0f)*0.8f;
        if(noisy) x += 0.04f*std::sin(i*1.731f);
        s.samples.setSample(0,i,x);
    }
    return s;
}

class AnalyzerTests final:public juce::UnitTest
{
public:
 AnalyzerTests():juce::UnitTest("SourceAnalyzer"){}
 void runTest() override
 {
  beginTest("analyzes a late transient across the source");
  auto s=makeSource();SourceAnalysis a;juce::String e;expect(SourceAnalyzer::analyze(s,a,e));expect(a.peak>0.1f);expect(a.rms>0);expect(a.fingerprint!=0);expect(a.strongestTransientSample>8000);
  beginTest("rejects silence");
  s.samples.clear();expect(!SourceAnalyzer::analyze(s,a,e));expect(e.isNotEmpty());
 }
};

class EngineTests final:public juce::UnitTest
{
public:
 EngineTests():juce::UnitTest("SnairEngine"){}
 void runTest() override
 {
  auto s=makeSource(true);SourceAnalysis a;juce::String e;expect(SourceAnalyzer::analyze(s,a,e));
  SnairParameters p;p.seed=42;
  beginTest("snare render is finite and bounded");
  auto h=SnairEngine::render(s,a,p,1,e);expect(h!=nullptr);expect(h->samples.getNumSamples()>0);expect(h->peak<=1.0001f);
  bool finite=true;for(int ch=0;ch<h->samples.getNumChannels();++ch)for(int i=0;i<h->samples.getNumSamples();++i)finite&=std::isfinite(h->samples.getSample(ch,i));expect(finite);
  beginTest("render is deterministic for same source state and seed");
  auto h2=SnairEngine::render(s,a,p,2,e);expect(h2!=nullptr);expectEquals(h->samples.getNumSamples(),h2->samples.getNumSamples());
  float diff=0;for(int i=0;i<h->samples.getNumSamples();++i)diff+=std::abs(h->samples.getSample(0,i)-h2->samples.getSample(0,i));expect(diff<1.0e-5f);
  beginTest("seed changes output");
  p.seed=43;auto h3=SnairEngine::render(s,a,p,3,e);float changed=0;for(int i=0;i<std::min(h->samples.getNumSamples(),h3->samples.getNumSamples());++i)changed+=std::abs(h->samples.getSample(0,i)-h3->samples.getSample(0,i));expect(changed>0.01f);
  beginTest("device-rate rendering preserves duration at requested sample rate");
  p.mode=SnairMode::snare;p.seed=42;auto rateHit=SnairEngine::render(s,a,p,4,e,44100.0);expect(rateHit!=nullptr);expectWithinAbsoluteError(rateHit->sampleRate,44100.0,0.01);expect(rateHit->samples.getNumSamples()>1000);
  beginTest("tone parameter materially changes the render");
  p.tone=-1.0f;auto dark=SnairEngine::render(s,a,p,5,e);p.tone=1.0f;auto bright=SnairEngine::render(s,a,p,6,e);float toneDiff=0;for(int i=0;i<std::min(dark->samples.getNumSamples(),bright->samples.getNumSamples());++i)toneDiff+=std::abs(dark->samples.getSample(0,i)-bright->samples.getSample(0,i));expect(toneDiff>0.01f);
  beginTest("clap mode generates deterministic multi-burst output");
  p.mode=SnairMode::clap;p.seed=99;p.clapCount=6;p.clapSpreadMs=35;auto c=SnairEngine::render(s,a,p,7,e);expect(c!=nullptr);expect(c->peak<=1.0001f);
  beginTest("randomize and mutate stay in bounds and preserve mode");
  const auto mode=p.mode;auto r=SnairEngine::randomized(p,1234);expect(r.mode==mode);expect(r.clapCount>=2&&r.clapCount<=6);expect(r.bodyFreqHz>=70&&r.bodyFreqHz<=450);
  auto m=SnairEngine::mutated(p,2222);expect(m.mode==mode);expect(m.sourceCharacter>=0&&m.sourceCharacter<=1);expect(m.driveDb>=0&&m.driveDb<=24);
 }
};

class IoTests final:public juce::UnitTest
{
public:
 IoTests():juce::UnitTest("Standalone IO"){}
 void runTest() override
 {
  auto s=makeSource(true);SourceAnalysis a;juce::String e;expect(SourceAnalyzer::analyze(s,a,e));
  SnairParameters p;p.seed=777;p.mode=SnairMode::clap;
  auto hit=SnairEngine::render(s,a,p,55,e,48000.0);expect(hit!=nullptr);
  const auto root=juce::File::getSpecialLocation(juce::File::tempDirectory)
      .getNonexistentChildFile("snaircreator-tests","",true);
  expect(root.createDirectory());

  beginTest("session recovery round-trips rendered hit and parameters");
  const auto fakeSource=root.getChildFile("moved-source.wav");
  expect(SessionStore::save(root,p,hit,fakeSource,e));expect(e.isEmpty());
  RestoredSession restored;expect(SessionStore::load(root,restored,e));expect(e.isEmpty());
  expect(restored.hit!=nullptr);expect(restored.parameters.mode==p.mode);expectEquals((int)restored.parameters.seed,(int)p.seed);
  expectEquals(restored.hit->samples.getNumSamples(),hit->samples.getNumSamples());
  expectWithinAbsoluteError(restored.hit->samples.getSample(0,100),hit->samples.getSample(0,100),1.0e-7f);
  expectEquals(restored.sourceFile.getFullPathName(),fakeSource.getFullPathName());

  beginTest("WAV export supports PCM16 PCM24 and float32 with resampling");
  juce::AudioFormatManager formats;formats.registerBasicFormats();
  for(const auto depth:{16,24,32})
  {
   WavExportOptions o;o.bitDepth=depth;o.sampleRate=44100.0;o.channels=1;o.normalize=true;
   const auto file=root.getChildFile("export-"+juce::String(depth)+".wav");
   expect(WavExporter::write(file,*hit,o,e));expect(e.isEmpty());
   std::unique_ptr<juce::AudioFormatReader> reader(formats.createReaderFor(file));
   expect(reader!=nullptr);
   if(reader)
   {
    expectWithinAbsoluteError(reader->sampleRate,44100.0,0.01);
    expectEquals((int)reader->numChannels,1);
    expectEquals((int)reader->bitsPerSample,depth);
    if(depth==32) expect(reader->usesFloatingPointData);
   }
  }
  expect(root.deleteRecursively());
 }
};

static AnalyzerTests analyzerTests;
static EngineTests engineTests;
static IoTests ioTests;
}
