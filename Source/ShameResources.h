#pragma once

#include "shameConfig.h"
#include "BinaryData.h"

// Loading helpers for the embedded GUI/audio resources.
namespace ShameResources
{
    inline Image getImage(const char* data, int size)
    {
        return ImageCache::getFromMemory(data, size);
    }

    struct LoadedAudio
    {
        AudioBuffer<float> buffer;
        double sourceSampleRate = 44100.0;
    };

    inline LoadedAudio loadAudio(const void* data, size_t size)
    {
        LoadedAudio result;

        AudioFormatManager formatManager;
        formatManager.registerBasicFormats();

        auto stream = std::make_unique<MemoryInputStream>(data, size, false);
        if (auto reader = std::unique_ptr<AudioFormatReader>(formatManager.createReaderFor(std::move(stream))))
        {
            result.buffer.setSize((int) reader->numChannels, (int) reader->lengthInSamples);
            reader->read(&result.buffer, 0, (int) reader->lengthInSamples, 0, true, true);
            result.sourceSampleRate = reader->sampleRate;
        }

        return result;
    }

    // Linear-phase-enough resampling for noise/ambience beds: keeps the hiss
    // and grain textures at the correct pitch and duration at any host rate.
    inline AudioBuffer<float> resampleTo(const AudioBuffer<float>& source, double sourceRate, double targetRate)
    {
        if (source.getNumSamples() == 0 || sourceRate <= 0.0 || targetRate <= 0.0
            || std::abs(sourceRate - targetRate) < 0.01)
            return source;

        const double ratio = sourceRate / targetRate;
        const int outLength = (int) std::floor((double) source.getNumSamples() / ratio);

        AudioBuffer<float> out(source.getNumChannels(), jmax(1, outLength));

        for (int ch = 0; ch < source.getNumChannels(); ++ch)
        {
            LagrangeInterpolator interpolator;
            interpolator.reset();
            interpolator.process(ratio, source.getReadPointer(ch), out.getWritePointer(ch), out.getNumSamples());
        }

        return out;
    }

    // The shared audio beds, loaded once and resampled per prepare().
    struct AudioAssets
    {
        AudioBuffer<float> hiss;
        AudioBuffer<float> pinkNoise;
        AudioBuffer<float> lowLevelGrainNoise;

        void prepare(double sampleRate)
        {
            if (std::abs(sampleRate - preparedRate) < 0.01 && hiss.getNumSamples() > 0)
                return;

            auto h = loadAudio(BinaryData::Hiss_wav, BinaryData::Hiss_wavSize);
            auto p = loadAudio(BinaryData::PinkNoise_wav, BinaryData::PinkNoise_wavSize);
            auto g = loadAudio(BinaryData::LowLevelGrainNoise_wav, BinaryData::LowLevelGrainNoise_wavSize);

            hiss               = resampleTo(h.buffer, h.sourceSampleRate, sampleRate);
            pinkNoise          = resampleTo(p.buffer, p.sourceSampleRate, sampleRate);
            lowLevelGrainNoise = resampleTo(g.buffer, g.sourceSampleRate, sampleRate);

            preparedRate = sampleRate;
        }

    private:
        double preparedRate = 0.0;
    };
}
