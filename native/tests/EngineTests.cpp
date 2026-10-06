#include <JuceHeader.h>
#include "Parameters.h"
#include "FactoryPresets.h"
#include "SourceLoader.h"
#include "SourceAnalyzer.h"
#include "SnairEngine.h"
#include "SessionStore.h"
#include "WavExporter.h"

namespace
{
bool writeTestAudio(const juce::File& file,bool aiff)
{
    juce::AudioBuffer<float> buffer(2,4800);
    for(int i=0;i<buffer.getNumSamples();++i)
    {
        const float x=0.35f*std::sin(juce::MathConstants<float>::twoPi*220.0f*(float)i/48000.0f);
        buffer.setSample(0,i,x);
        buffer.setSample(1,i,x*0.8f);
    }
    std::unique_ptr<juce::OutputStream> stream=file.createOutputStream();
    if(!stream) return false;
    auto options=juce::AudioFormatWriter::Options{}.withSampleRate(48000.0).withNumChannels(2).withBitsPerSample(24);
    std::unique_ptr<juce::AudioFormatWriter> writer;
    if(aiff)
    {
        juce::AiffAudioFormat format;
        writer=format.createWriterFor(stream,options);
    }
    else
    {
        juce::WavAudioFormat format;
        writer=format.createWriterFor(stream,options);
    }
    if(!writer) return false;
    return writer->writeFromAudioSampleBuffer(buffer,0,buffer.getNumSamples());
}

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

  beginTest("analysis removes DC and provides complete descriptors");
  s=makeSource();
  for(int i=0;i<s.samples.getNumSamples();++i) s.samples.addSample(0,i,0.2f);
  expect(SourceAnalyzer::analyze(s,a,e));
  expectWithinAbsoluteError(a.dcOffset,0.2f,0.02f);
  expect(a.spectralRolloffHz>0.0f);expect(a.spectralFlatness>=0.0f&&a.spectralFlatness<=1.0f);
  expect(a.transientDensityPerSecond>0.0f);expect(a.attackWindowEnd>a.attackWindowStart);
  expect(a.tailWindowEnd>=a.tailWindowStart);
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

class SourceLoaderTests final:public juce::UnitTest
{
public:
 SourceLoaderTests():juce::UnitTest("SourceLoader"){}
 void runTest() override
 {
  const auto root=juce::File::getSpecialLocation(juce::File::tempDirectory)
      .getNonexistentChildFile("snaircreator-source-tests","",true);
  expect(root.createDirectory());
  juce::String e;
  beginTest("decodes WAV and AIFF with stereo metadata");
  const auto wav=root.getChildFile("source.wav");const auto aiff=root.getChildFile("source.aiff");
  expect(writeTestAudio(wav,false));expect(writeTestAudio(aiff,true));
  auto w=SourceLoader::decode(wav,e);expect(w!=nullptr);expectEquals(w?w->channelCount:0,2);expect(w&&w->fileSizeBytes>0);
  auto a=SourceLoader::decode(aiff,e);expect(a!=nullptr);expectEquals(a?a->channelCount:0,2);
  beginTest("corrupt replacement is rejected");
  const auto bad=root.getChildFile("bad.wav");expect(bad.replaceWithText("not audio"));
  auto invalid=SourceLoader::decode(bad,e);expect(invalid==nullptr);expect(e.isNotEmpty());
  expect(root.deleteRecursively());
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

class PresetRecipeTests final:public juce::UnitTest
{
public:
 PresetRecipeTests():juce::UnitTest("Factory presets"){}
 void runTest() override
 {
  beginTest("factory recipes are bounded and have unique names");
  juce::StringArray names;
  for(const auto& preset:FactoryPresets::all())
  {
   auto p=preset.parameters;p.sanitize();
   expect(p==preset.parameters);expect(!names.contains(preset.name));names.add(preset.name);
  }
  expect(FactoryPresets::all().size()>=8);
 }
};

static AnalyzerTests analyzerTests;
static EngineTests engineTests;
static SourceLoaderTests sourceLoaderTests;
static IoTests ioTests;
static PresetRecipeTests presetRecipeTests;
}
