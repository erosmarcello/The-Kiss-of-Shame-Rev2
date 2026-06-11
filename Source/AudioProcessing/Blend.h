//
//  Blend.h
//  KissOfShame
//
//  Created by Brian Hansen on 9/14/14.
//  Rev 2: unchanged math, channel-safe.
//

#pragma once

#include "../shameConfig.h"

class Blend
{
public:
    void processBlend(AudioSampleBuffer& dryBuffer, const AudioSampleBuffer& wetBuffer, int numChannels)
    {
        numChannels = jmin(numChannels, jmin(dryBuffer.getNumChannels(), wetBuffer.getNumChannels()));

        for (int channel = 0; channel < numChannels; ++channel)
        {
            float* samples = dryBuffer.getWritePointer(channel);
            const float* wet = wetBuffer.getReadPointer(channel);

            for (int i = 0; i < dryBuffer.getNumSamples(); ++i)
                samples[i] = (1.0f - blendValue) * samples[i] + blendValue * wet[i];
        }
    }

    void setBlendLevel(float level) { blendValue = jlimit(0.0f, 1.0f, level); }

private:
    float blendValue = 0.0f;
};
