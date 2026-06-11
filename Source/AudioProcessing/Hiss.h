//
//  Hiss.h
//  KissOfShame
//
//  Created by Brian Hansen on 9/9/14.
//  Rev 2: embedded resource, crash-proof (the Rev 1 build divided by zero
//  whenever Hiss.wav was missing from the hard-coded path), channel-safe,
//  tape-type aware scaling.
//
//  Two read heads half a loop apart, equal-power-ish ramped, so the hiss
//  bed never audibly loops.
//

#pragma once

#include "../shameConfig.h"

class Hiss
{
public:
    Hiss() = default;

    // The hiss bed arrives already resampled to the host rate.
    void setAudioData(const AudioBuffer<float>& hissBed)
    {
        hissBuffer = hissBed;
        indx1 = 0;
        indx2 = hissBuffer.getNumSamples() / 2;
    }

    void reset()
    {
        indx1 = 0;
        indx2 = hissBuffer.getNumSamples() / 2;
    }

    void setHissLevel(float level)
    {
        hissLevel = level * 0.005f * tapeTypeScale;
        signalLevel = 1.0f - hissLevel;
    }

    // A-456 is the high-output/low-noise formulation: same control range,
    // quieter floor.
    void setTapeType(bool useA456)
    {
        tapeTypeScale = useA456 ? 0.6f : 1.0f;
    }

    void process(AudioSampleBuffer& sampleBuffer, int numChannels)
    {
        const int hissLength = hissBuffer.getNumSamples();
        if (hissLength < 2)
            return;

        const int hissChannels = hissBuffer.getNumChannels();

        for (int i = 0; i < sampleBuffer.getNumSamples(); ++i)
        {
            float rampValue;
            if (indx1 < hissLength / 2)
                rampValue = (float) indx1 / (0.5f * (float) hissLength);
            else
                rampValue = (float) (hissLength - indx1) / (0.5f * (float) hissLength);

            for (int channel = 0; channel < numChannels; ++channel)
            {
                const int hissChannel = jmin(channel, hissChannels - 1);
                const float* hissData = hissBuffer.getReadPointer(hissChannel);
                const float hissSample = rampValue * hissData[indx1] + (1.0f - rampValue) * hissData[indx2];

                float* samples = sampleBuffer.getWritePointer(channel);
                samples[i] = signalLevel * samples[i] + hissLevel * hissSample;
            }

            indx1 = (indx1 + 1) % hissLength;
            indx2 = (indx2 + 1) % hissLength;
        }
    }

private:
    AudioBuffer<float> hissBuffer;
    int indx1 = 0;
    int indx2 = 0;

    float hissLevel = 0.0f;
    float signalLevel = 1.0f;
    float tapeTypeScale = 1.0f;
};
