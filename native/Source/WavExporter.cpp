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
        const float left=hit.samples.getSample(0,i);
        const float right=hit.samples.getNumChannels()>1?hit.samples.getSample(1,i):left;
        if(outChannels==1)
            data.setSample(0,i,0.5f*(left+right));
        else
        {
            data.setSample(0,i,left);
            data.setSample(1,i,right);
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
        const int newSamples=std::max(1,static_cast<int>(std::ceil(data.getNumSamples()/speedRatio)));
        juce::AudioBuffer<float> resampled(outChannels,newSamples);
        for(int ch=0;ch<outChannels;++ch)
        {
            juce::WindowedSincInterpolator interpolator;
            interpolator.process(speedRatio,
                                 data.getReadPointer(ch),
                                 resampled.getWritePointer(ch),
                                 newSamples,
                                 data.getNumSamples(),
                                 0);
        }
        data=std::move(resampled);
    }

    const auto parent=file.getParentDirectory();
    if(!parent.exists() && !parent.createDirectory())
    {
        error="The export destination folder is not writable.";
        return false;
    }

    juce::TemporaryFile temporary(file);
    std::unique_ptr<juce::OutputStream> stream=temporary.getFile().createOutputStream();
    if(!stream)
    {
        error="Could not create a temporary WAV in the export folder.";
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
    writer.reset();

    std::unique_ptr<juce::AudioFormatReader> reader(
        wav.createReaderFor(temporary.getFile().createInputStream().release(),true));
    if(!reader)
    {
        error="The exported WAV could not be reopened for validation.";
        return false;
    }

    const bool metadataMatches=std::abs(reader->sampleRate-outRate)<0.5
        && static_cast<int>(reader->numChannels)==outChannels
        && static_cast<int>(reader->bitsPerSample)==bitDepth
        && reader->lengthInSamples==data.getNumSamples()
        && (bitDepth!=32 || reader->usesFloatingPointData);
    if(!metadataMatches)
    {
        error="The exported WAV failed format validation.";
        return false;
    }
    reader.reset();

    if(!temporary.overwriteTargetFileWithTemporary())
    {
        error="The validated WAV could not replace the destination file.";
        return false;
    }

    return true;
}
