#include "SessionStore.h"
#include "PresetManager.h"
#include <cmath>

namespace
{
constexpr char magic[]="SNAIRHIT1";
constexpr int magicBytes=9;
constexpr int version=1;
constexpr int maxChannels=2;
constexpr int maxSamplesPerChannel=2000000;

bool writeHit(const juce::File& file,const RenderedHit& hit,juce::String& error)
{
    const int channels=hit.samples.getNumChannels();
    const int samples=hit.samples.getNumSamples();
    if(channels<1 || channels>maxChannels || samples<1 || samples>maxSamplesPerChannel
       || !std::isfinite(hit.sampleRate) || hit.sampleRate<8000.0)
    {
        error="The active render cannot be serialized safely.";
        return false;
    }

    if(file.existsAsFile() && !file.deleteFile())
    {
        error="The previous recovery audio could not be replaced.";
        return false;
    }

    auto stream=file.createOutputStream();
    if(!stream || !stream->openedOk())
    {
        error="Could not create recovery audio.";
        return false;
    }

    stream->write(magic,magicBytes);
    stream->writeInt(version);
    stream->writeDouble(hit.sampleRate);
    stream->writeInt(channels);
    stream->writeInt(samples);
    stream->writeInt64(static_cast<int64_t>(hit.sourceFingerprint));
    stream->writeInt(static_cast<int>(hit.seed));
    stream->writeInt64(static_cast<int64_t>(hit.generationId));
    stream->writeFloat(hit.peak);
    stream->writeByte(hit.usedFallback?1:0);

    for(int ch=0;ch<channels;++ch)
        for(int i=0;i<samples;++i)
            stream->writeFloat(hit.samples.getSample(ch,i));

    stream->flush();
    if(stream->getStatus().failed())
    {
        error="Recovery audio could not be fully written.";
        return false;
    }
    return true;
}

RenderedHitPtr readHit(const juce::File& file,juce::String& error)
{
    if(!file.existsAsFile()) return {};

    juce::FileInputStream stream(file);
    if(!stream.openedOk())
    {
        error="Recovery audio could not be opened.";
        return {};
    }

    char header[magicBytes]{};
    if(stream.read(header,magicBytes)!=magicBytes
       || std::memcmp(header,magic,magicBytes)!=0
       || stream.readInt()!=version)
    {
        error="Recovery audio has an unsupported format.";
        return {};
    }

    const double sampleRate=stream.readDouble();
    const int channels=stream.readInt();
    const int samples=stream.readInt();
    const auto fingerprint=static_cast<uint64_t>(stream.readInt64());
    const auto seed=static_cast<uint32_t>(stream.readInt());
    const auto generation=static_cast<uint64_t>(stream.readInt64());
    const float storedPeak=stream.readFloat();
    const bool fallback=stream.readByte()!=0;

    if(!std::isfinite(sampleRate) || sampleRate<8000.0 || channels<1 || channels>maxChannels
       || samples<1 || samples>maxSamplesPerChannel)
    {
        error="Recovery audio metadata is invalid or too large.";
        return {};
    }

    auto hit=std::make_shared<RenderedHit>();
    hit->sampleRate=sampleRate;
    hit->sourceFingerprint=fingerprint;
    hit->seed=seed;
    hit->generationId=generation;
    hit->usedFallback=fallback;
    hit->samples.setSize(channels,samples);

    float actualPeak=0.0f;
    for(int ch=0;ch<channels;++ch)
    {
        for(int i=0;i<samples;++i)
        {
            float value=stream.readFloat();
            if(!std::isfinite(value)) value=0.0f;
            hit->samples.setSample(ch,i,value);
            actualPeak=std::max(actualPeak,std::abs(value));
        }
    }

    if(stream.getStatus().failed())
    {
        error="Recovery audio is truncated or corrupt.";
        return {};
    }

    hit->peak=std::isfinite(storedPeak)?std::max(storedPeak,actualPeak):actualPeak;
    return hit;
}
}

juce::File SessionStore::defaultDirectory()
{
    return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
        .getChildFile("Circuit Drift Labs")
        .getChildFile("SnairCreator");
}

bool SessionStore::save(const juce::File& directory,
                        const SnairParameters& parameters,
                        const RenderedHitPtr& hit,
                        const juce::File& sourceFile,
                        juce::String& error)
{
    error.clear();
    if(!directory.createDirectory())
    {
        error="Could not create the SnairCreator recovery folder.";
        return false;
    }

    if(!PresetManager::save(directory.getChildFile("session.json"),parameters,error))
        return false;

    if(!directory.getChildFile("source.path").replaceWithText(sourceFile.getFullPathName()))
    {
        error="Could not save the source recovery path.";
        return false;
    }

    const auto hitFile=directory.getChildFile("current.hit");
    if(hit)
        return writeHit(hitFile,*hit,error);

    if(hitFile.existsAsFile()) hitFile.deleteFile();
    return true;
}

bool SessionStore::load(const juce::File& directory,
                        RestoredSession& session,
                        juce::String& error)
{
    error.clear();
    const auto stateFile=directory.getChildFile("session.json");
    if(!stateFile.existsAsFile()) return false;

    RestoredSession candidate;
    if(!PresetManager::load(stateFile,candidate.parameters,error))
        return false;

    const auto pathFile=directory.getChildFile("source.path");
    if(pathFile.existsAsFile())
    {
        const auto path=pathFile.loadFileAsString().trim();
        if(path.isNotEmpty()) candidate.sourceFile=juce::File(path);
    }

    candidate.hit=readHit(directory.getChildFile("current.hit"),error);
    if(error.isNotEmpty()) return false;

    session=std::move(candidate);
    return true;
}
