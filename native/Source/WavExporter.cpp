#include "WavExporter.h"
#include <memory>

bool WavExporter::write(const juce::File& file,const RenderedHit& hit,const WavExportOptions& opt,juce::String& error)
{
    error.clear();
    if(hit.samples.getNumSamples()==0){ error="There is no rendered hit to export."; return false; }
    const double outRate=opt.sampleRate>0.0?opt.sampleRate:hit.sampleRate;
    const int outChannels=std::clamp(opt.channels,1,2);
    const int bitDepth=(opt.bitDepth==16||opt.bitDepth==24||opt.bitDepth==32)?opt.bitDepth:24;

    juce::AudioBuffer<float> data(outChannels,hit.samples.getNumSamples());
    for(int i=0;i<data.getNumSamples();++i)
    {
        const float l=hit.samples.getSample(0,i);
        const float r=hit.samples.getNumChannels()>1?hit.samples.getSample(1,i):l;
        if(outChannels==1) data.setSample(0,i,0.5f*(l+r));
        else { data.setSample(0,i,l); data.setSample(1,i,r); }
    }

    if(opt.includeOutputTrim) data.applyGain(std::pow(10.0f,opt.outputTrimDb/20.0f));
    if(opt.normalize)
    {
        const float peak=data.getMagnitude(0,data.getNumSamples());
        if(peak>1.0e-7f) data.applyGain(0.98f/peak);
    }

    if(outRate!=hit.sampleRate)
    {
        const double ratio=outRate/hit.sampleRate;
        const int newN=std::max(1,static_cast<int>(std::ceil(data.getNumSamples()*ratio)));
        juce::AudioBuffer<float> resampled(outChannels,newN);
        for(int ch=0;ch<outChannels;++ch)
        {
            juce::LagrangeInterpolator interp;
            interp.process(1.0/ratio,data.getReadPointer(ch),resampled.getWritePointer(ch),newN);
        }
        data=std::move(resampled);
    }

    if(file.existsAsFile() && !file.deleteFile()){ error="The existing export file could not be replaced."; return false; }
    auto stream=std::unique_ptr<juce::FileOutputStream>(file.createOutputStream());
    if(!stream || !stream->openedOk()){ error="Could not open the export file for writing."; return false; }

    juce::WavAudioFormat wav;
    std::unique_ptr<juce::AudioFormatWriter> writer(wav.createWriterFor(stream.release(),outRate,static_cast<unsigned int>(outChannels),bitDepth,{},0));
    if(!writer){ error="Could not create the WAV encoder."; return false; }
    if(!writer->writeFromAudioSampleBuffer(data,0,data.getNumSamples())){ error="WAV encoding failed."; return false; }
    return true;
}
