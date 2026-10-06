#include "WavExporter.h"
#include <memory>

bool WavExporter::write(const juce::File& file,const RenderedHit& hit,const WavExportOptions& opt,juce::String& error)
{
    error.clear();
    if(hit.samples.getNumSamples()==0)
    {
        error="There is no rendered hit to export.";
        return false;
    }

    const double outRate=opt.sampleRate>0.0?opt.sampleRate:hit.sampleRate;
    const int outChannels=std::clamp(opt.channels,1,2);
    const int bitDepth=(opt.bitDepth==16||opt.bitDepth==24||opt.bitDepth==32)?opt.bitDepth:24;

    juce::AudioBuffer<float> data(outChannels,hit.samples.getNumSamples());
    for(int i=0;i<data.getNumSamples();++i)
    {
        const float l=hit.samples.getSample(0,i);
        const float r=hit.samples.getNumChannels()>1?hit.samples.getSample(1,i):l;
        if(outChannels==1)
            data.setSample(0,i,0.5f*(l+r));
        else
        {
            data.setSample(0,i,l);
            data.setSample(1,i,r);
        }
    }

    if(opt.includeOutputTrim)
        data.applyGain(std::pow(10.0f,opt.outputTrimDb/20.0f));

    if(opt.normalize)
    {
        const float peak=data.getMagnitude(0,data.getNumSamples());
        if(peak>1.0e-7f) data.applyGain(0.98f/peak);
    }

    if(std::abs(outRate-hit.sampleRate)>0.5)
    {
        const double speedRatio=hit.sampleRate/outRate;
        const int newN=std::max(1,static_cast<int>(std::ceil(data.getNumSamples()/speedRatio)));
        juce::AudioBuffer<float> resampled(outChannels,newN);
        for(int ch=0;ch<outChannels;++ch)
        {
            juce::WindowedSincInterpolator interp;
            interp.process(speedRatio,
                           data.getReadPointer(ch),
                           resampled.getWritePointer(ch),
                           newN,
                           data.getNumSamples(),
                           0);
        }
        data=std::move(resampled);
    }

    if(file.existsAsFile() && !file.deleteFile())
    {
        error="The existing export file could not be replaced.";
        return false;
    }

    std::unique_ptr<juce::OutputStream> stream=file.createOutputStream();
    if(!stream)
    {
        error="Could not open the export file for writing.";
        return false;
    }

    juce::WavAudioFormat wav;
    auto options=juce::AudioFormatWriter::Options{}
        .withSampleRate(outRate)
        .withNumChannels(outChannels)
        .withBitsPerSample(bitDepth)
        .withSampleFormat(bitDepth==32
            ? juce::AudioFormatWriter::Options::SampleFormat::floatingPoint
            : juce::AudioFormatWriter::Options::SampleFormat::integral);

    auto writer=wav.createWriterFor(stream,options);
    if(!writer)
    {
        error="Could not create the WAV encoder for the selected format.";
        return false;
    }

    if(!writer->writeFromAudioSampleBuffer(data,0,data.getNumSamples()))
    {
        error="WAV encoding failed.";
        return false;
    }

    return true;
}
