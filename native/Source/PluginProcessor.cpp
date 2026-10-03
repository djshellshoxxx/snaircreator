#include "PluginProcessor.h"
#include "PluginEditor.h"

SnairCreatorAudioProcessor::SnairCreatorAudioProcessor()
    : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "PARAMS", createParameterLayout()) {
    for (auto id : {"mode","seed","character","body","body_freq_hz","crack","noise","tail_ms","clap_count","clap_spread_ms","width","drive_db","tone","pitch_st","trim_db","normalize","output_ms"})
        apvts.addParameterListener(id, this);
}

SnairCreatorAudioProcessor::~SnairCreatorAudioProcessor() {
    cancelPendingUpdate();
    for (auto id : {"mode","seed","character","body","body_freq_hz","crack","noise","tail_ms","clap_count","clap_spread_ms","width","drive_db","tone","pitch_st","trim_db","normalize","output_ms"})
        apvts.removeParameterListener(id, this);
}

juce::AudioProcessorValueTreeState::ParameterLayout SnairCreatorAudioProcessor::createParameterLayout() {
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout p;
    p.add(std::make_unique<AudioParameterChoice>(ParameterID{"mode",1}, "Mode", StringArray{"Snare","Clap","Hybrid"}, 0));
    p.add(std::make_unique<AudioParameterInt>(ParameterID{"seed",1}, "Seed", 0, 2147483647, 1));
    p.add(std::make_unique<AudioParameterFloat>(ParameterID{"character",1}, "Character", NormalisableRange<float>(0,1,.001f), .72f));
    p.add(std::make_unique<AudioParameterFloat>(ParameterID{"body",1}, "Body", NormalisableRange<float>(0,1,.001f), .68f));
    p.add(std::make_unique<AudioParameterFloat>(ParameterID{"body_freq_hz",1}, "Body Frequency", NormalisableRange<float>(70,450,.1f,.45f), 185.0f));
    p.add(std::make_unique<AudioParameterFloat>(ParameterID{"crack",1}, "Crack", NormalisableRange<float>(0,1,.001f), .72f));
    p.add(std::make_unique<AudioParameterFloat>(ParameterID{"noise",1}, "Noise", NormalisableRange<float>(0,1,.001f), .62f));
    p.add(std::make_unique<AudioParameterFloat>(ParameterID{"tail_ms",1}, "Tail", NormalisableRange<float>(20,1200,1,.5f), 260.0f));
    p.add(std::make_unique<AudioParameterInt>(ParameterID{"clap_count",1}, "Clap Count", 2, 6, 4));
    p.add(std::make_unique<AudioParameterFloat>(ParameterID{"clap_spread_ms",1}, "Clap Spread", NormalisableRange<float>(8,35,.1f), 18.0f));
    p.add(std::make_unique<AudioParameterFloat>(ParameterID{"width",1}, "Width", NormalisableRange<float>(0,1,.001f), .35f));
    p.add(std::make_unique<AudioParameterFloat>(ParameterID{"drive_db",1}, "Drive", NormalisableRange<float>(0,18,.1f), 4.0f));
    p.add(std::make_unique<AudioParameterFloat>(ParameterID{"tone",1}, "Tone", NormalisableRange<float>(-1,1,.001f), 0.0f));
    p.add(std::make_unique<AudioParameterFloat>(ParameterID{"pitch_st",1}, "Pitch", NormalisableRange<float>(-24,24,.1f), 0.0f));
    p.add(std::make_unique<AudioParameterFloat>(ParameterID{"trim_db",1}, "Trim", NormalisableRange<float>(-24,12,.1f), 0.0f));
    p.add(std::make_unique<AudioParameterBool>(ParameterID{"normalize",1}, "Normalize", true));
    p.add(std::make_unique<AudioParameterFloat>(ParameterID{"output_ms",1}, "Output Length", NormalisableRange<float>(40,2000,1,.5f), 420.0f));
    return p;
}

void SnairCreatorAudioProcessor::prepareToPlay(double sampleRate, int) {
    renderSampleRate = sampleRate > 1000.0 ? sampleRate : 48000.0;
    if (!source.empty()) triggerAsyncUpdate();
}

bool SnairCreatorAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

snair::Params SnairCreatorAudioProcessor::getParams() const {
    auto v = [this](const char* id) { return apvts.getRawParameterValue(id)->load(); };
    snair::Params p;
    p.mode=static_cast<int>(v("mode")); p.seed=static_cast<int>(v("seed")); p.character=v("character"); p.body=v("body"); p.bodyFreqHz=v("body_freq_hz");
    p.crack=v("crack"); p.noise=v("noise"); p.tailMs=v("tail_ms"); p.clapCount=static_cast<int>(v("clap_count")); p.clapSpreadMs=v("clap_spread_ms");
    p.width=v("width"); p.driveDb=v("drive_db"); p.tone=v("tone"); p.pitchSt=v("pitch_st"); p.trimDb=v("trim_db"); p.normalize=v("normalize")>=.5f; p.outputMs=v("output_ms");
    return snair::SnairEngine::sanitize(p);
}

bool SnairCreatorAudioProcessor::loadSourceFile(const juce::File& file) {
    juce::AudioFormatManager formats; formats.registerBasicFormats();
    std::unique_ptr<juce::AudioFormatReader> reader(formats.createReaderFor(file));
    if (!reader || reader->lengthInSamples <= 0) return false;
    const auto maxFrames = static_cast<juce::int64>(reader->sampleRate * 60.0);
    const int frames = static_cast<int>(std::min(reader->lengthInSamples, maxFrames));
    juce::AudioBuffer<float> temp(static_cast<int>(reader->numChannels), frames);
    if (!reader->read(&temp, 0, frames, 0, true, true)) return false;
    source.assign(static_cast<size_t>(frames), 0.0f);
    for (int c=0;c<temp.getNumChannels();++c)
        for (int i=0;i<frames;++i) source[static_cast<size_t>(i)] += temp.getSample(c,i) / temp.getNumChannels();
    sourceSampleRate=reader->sampleRate; sourceFile=file; missingSourcePath.clear(); analysis=snair::SnairEngine::analyse(source,sourceSampleRate);
    if (auto* f=apvts.getParameter("body_freq_hz")) f->setValueNotifyingHost(f->convertTo0to1(analysis.bodyFreqHint));
    rebuildRendered();
    return true;
}

std::vector<float> SnairCreatorAudioProcessor::sourceAtRenderRate() const {
    if (source.empty()) return {};
    if (std::abs(sourceSampleRate-renderSampleRate) < 0.01) return source;
    const auto outputFrames=static_cast<size_t>(std::max(1.0,std::round(source.size()*renderSampleRate/sourceSampleRate)));
    std::vector<float> out(outputFrames,0.0f);
    const double step=sourceSampleRate/renderSampleRate;
    for(size_t i=0;i<outputFrames;++i){const double pos=static_cast<double>(i)*step;const size_t i0=std::min(source.size()-1,static_cast<size_t>(pos));const size_t i1=std::min(source.size()-1,i0+1);const float frac=static_cast<float>(pos-static_cast<double>(i0));out[i]=source[i0]*(1.0f-frac)+source[i1]*frac;}
    return out;
}

void SnairCreatorAudioProcessor::rebuildRendered() {
    if (source.empty()) return;
    auto renderSource=sourceAtRenderRate();
    auto next=snair::SnairEngine::render(renderSource,renderSampleRate,getParams());
    const juce::SpinLock::ScopedLockType lock(renderedLock); rendered.swap(next);
}

void SnairCreatorAudioProcessor::parameterChanged(const juce::String&, float) { triggerAsyncUpdate(); }
void SnairCreatorAudioProcessor::handleAsyncUpdate() { if(!source.empty()) rebuildRendered(); }

void SnairCreatorAudioProcessor::startVoice(float velocity) {
    Voice* chosen=&voices[0]; for(auto& voice:voices) if(!voice.active){chosen=&voice;break;}
    chosen->active=true; chosen->position=0; chosen->velocity=juce::jlimit(0.0f,1.0f,velocity);
}

void SnairCreatorAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) {
    juce::ScopedNoDenormals noDenormals; buffer.clear();
    if (previewRequested.exchange(false)) startVoice(1.0f);
    for (const auto metadata:midi) { const auto msg=metadata.getMessage(); if(msg.isNoteOn()) startVoice(msg.getFloatVelocity()); }
    if (!renderedLock.tryEnter()) return;
    if (rendered.empty()) { renderedLock.exit(); return; }
    for (int s=0;s<buffer.getNumSamples();++s) {
        float mixed=0.0f;
        for(auto& voice:voices) if(voice.active) {
            if(voice.position<rendered.size()) mixed += rendered[voice.position++] * voice.velocity; else voice.active=false;
        }
        mixed=juce::jlimit(-1.0f,1.0f,mixed);
        for(int c=0;c<buffer.getNumChannels();++c) buffer.setSample(c,s,mixed);
    }
    renderedLock.exit();
}

bool SnairCreatorAudioProcessor::exportRendered(const juce::File& file, int bitDepth) const {
    std::vector<float> snapshot;
    { const juce::SpinLock::ScopedLockType lock(renderedLock); snapshot=rendered; }
    if(snapshot.empty()) return false;
    file.deleteFile(); juce::WavAudioFormat wav; auto stream=file.createOutputStream(); if(!stream) return false;
    auto* rawStream=stream.release();
    auto writer=std::unique_ptr<juce::AudioFormatWriter>(wav.createWriterFor(rawStream,renderSampleRate,1u,bitDepth==16?16:24,{},0));
    if(!writer){delete rawStream;return false;}
    juce::AudioBuffer<float> b(1,static_cast<int>(snapshot.size())); b.copyFrom(0,0,snapshot.data(),static_cast<int>(snapshot.size()));
    return writer->writeFromAudioSampleBuffer(b,0,b.getNumSamples());
}

juce::String SnairCreatorAudioProcessor::getSourceDescription() const {
    if(!missingSourcePath.isEmpty()) return "Source missing — reload: " + missingSourcePath;
    if(source.empty()) return "No source loaded";
    return sourceFile.getFileName()+"  |  "+juce::String(analysis.duration,2)+" s  |  "+juce::String(sourceSampleRate,0)+" Hz  |  RMS "+juce::String(analysis.rms,3)+"  |  body "+juce::String(analysis.bodyFreqHint,0)+" Hz";
}

std::vector<float> SnairCreatorAudioProcessor::getSourcePreview(int points) const {
    const int count=juce::jlimit(16,2048,points); std::vector<float> out(static_cast<size_t>(count),0.0f); if(source.empty()) return out;
    const size_t step=std::max<size_t>(1,source.size()/static_cast<size_t>(count));
    for(int p=0;p<count;++p){const size_t start=static_cast<size_t>(p)*step,end=std::min(source.size(),start+step);float peak=0;for(size_t i=start;i<end;++i)if(std::abs(source[i])>std::abs(peak))peak=source[i];out[static_cast<size_t>(p)]=peak;}
    return out;
}

void SnairCreatorAudioProcessor::getStateInformation(juce::MemoryBlock& dest) {
    auto state=apvts.copyState();
    state.setProperty("sourcePath",sourceFile.getFullPathName().isNotEmpty()?sourceFile.getFullPathName():missingSourcePath,nullptr);
    std::unique_ptr<juce::XmlElement> xml(state.createXml()); copyXmlToBinary(*xml,dest);
}

void SnairCreatorAudioProcessor::setStateInformation(const void* data, int size) {
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data,size)); if(!xml) return;
    auto state=juce::ValueTree::fromXml(*xml); if(!state.isValid()) return;
    const auto path=state.getProperty("sourcePath").toString(); apvts.replaceState(state);
    if(path.isNotEmpty()) {
        juce::File f(path);
        if(f.existsAsFile()) loadSourceFile(f);
        else { source.clear(); sourceFile={}; missingSourcePath=path; const juce::SpinLock::ScopedLockType lock(renderedLock); rendered.clear(); }
    }
}

juce::AudioProcessorEditor* SnairCreatorAudioProcessor::createEditor(){return new SnairCreatorAudioProcessorEditor(*this);}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter(){return new SnairCreatorAudioProcessor();}
