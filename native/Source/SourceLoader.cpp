#include "SourceLoader.h"

#include <algorithm>
#include <cmath>
#include <limits>

std::shared_ptr<SourceAudio> SourceLoader::decode(const juce::File& file,
                                                   juce::String& errorMessage)
{
    errorMessage.clear();

    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats();

    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(file));
    if (reader == nullptr)
    {
        errorMessage = "The file could not be decoded as supported audio.";
        return {};
    }

    if (reader->lengthInSamples <= 0 || !std::isfinite(reader->sampleRate)
        || reader->sampleRate <= 0.0)
    {
        errorMessage = "The audio file has no usable samples or sample rate.";
        return {};
    }

    if (reader->numChannels == 0 || reader->numChannels > 2)
    {
        errorMessage = "This build accepts mono or stereo audio. Convert files with more than two channels to mono or stereo.";
        return {};
    }

    const auto duration = static_cast<double>(reader->lengthInSamples) / reader->sampleRate;
    if (!std::isfinite(duration) || duration > maxDurationSeconds)
    {
        errorMessage = "The source exceeds the 10 minute limit for this build.";
        return {};
    }

    const auto channelCount = static_cast<uint64_t>(reader->numChannels);
    const auto frameCount = static_cast<uint64_t>(reader->lengthInSamples);
    constexpr auto bytesPerSample = static_cast<uint64_t>(sizeof(float));
    if (frameCount > maxDecodedBytes / bytesPerSample / channelCount)
    {
        errorMessage = "The decoded source would exceed the 256 MiB memory limit.";
        return {};
    }

    if (reader->lengthInSamples > std::numeric_limits<int>::max())
    {
        errorMessage = "The source is too long to load on this build.";
        return {};
    }

    auto source = std::make_shared<SourceAudio>();
    source->sourceFile = file;
    source->sampleRate = reader->sampleRate;
    source->channelCount = static_cast<int>(reader->numChannels);
    source->frameCount = reader->lengthInSamples;
    source->samples.setSize(source->channelCount, static_cast<int>(source->frameCount));

    if (!reader->read(&source->samples,
                      0,
                      static_cast<int>(source->frameCount),
                      0,
                      true,
                      source->channelCount > 1))
    {
        errorMessage = "The audio file could not be fully decoded. The previous source was kept.";
        return {};
    }

    for (int channel = 0; channel < source->channelCount; ++channel)
    {
        auto* samples = source->samples.getWritePointer(channel);
        for (int frame = 0; frame < source->samples.getNumSamples(); ++frame)
        {
            if (!std::isfinite(samples[frame]))
            {
                samples[frame] = 0.0f;
                ++source->sanitizedSampleCount;
            }
        }
    }

    source->waveformPeaks.assign(waveformBinCount, 0.0f);
    const auto totalFrames = static_cast<uint64_t>(source->frameCount);
    for (uint64_t frame = 0; frame < totalFrames; ++frame)
    {
        const auto bin = std::min(waveformBinCount - 1,
                                  static_cast<size_t>((frame * waveformBinCount) / totalFrames));
        float peak = 0.0f;
        for (int channel = 0; channel < source->channelCount; ++channel)
            peak = std::max(peak, std::abs(source->samples.getSample(channel, static_cast<int>(frame))));
        source->waveformPeaks[bin] = std::max(source->waveformPeaks[bin], peak);
    }

    return source;
}
