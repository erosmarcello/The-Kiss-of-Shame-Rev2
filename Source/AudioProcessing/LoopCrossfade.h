//
//  LoopCrossfade.h
//  KissOfShame
//
//  Created by Brian Hansen on 9/9/14.
//  Rev 2: embedded resource data, guarded against empty buffers and
//  channel overruns (the Rev 1 version modulo'd by zero when its WAV was
//  missing from the hard-coded path).
//

#pragma once

#include "../shameConfig.h"

class LoopCrossfade
{
public:
    LoopCrossfade() = default;

    void setAudioData(const AudioBuffer<float>& loopBed)
    {
        buffer = loopBed;
        indx1 = 0;
        indx2 = buffer.getNumSamples() / 2;
    }

    void reset()
    {
        indx1 = 0;
        indx2 = buffer.getNumSamples() / 2;
    }

    float processLoopCrossSample(int channel)
    {
        const int length = buffer.getNumSamples();
        if (length < 2)
            return 0.0f;

        float rampValue;
        if (indx1 < length / 2)
            rampValue = (float) indx1 / (0.5f * (float) length);
        else
            rampValue = (float) (length - indx1) / (0.5f * (float) length);

        const int ch = jmin(channel, buffer.getNumChannels() - 1);
        const float* data = buffer.getReadPointer(ch);
        const float sample = rampValue * data[indx1] + (1.0f - rampValue) * data[indx2];

        indx1 = (indx1 + 1) % length;
        indx2 = (indx2 + 1) % length;

        return loopCrossfadeLevel * sample;
    }

    void setLoopCrossfadeLevel(float level)
    {
        loopCrossfadeLevel = level;
    }

private:
    AudioBuffer<float> buffer;
    int indx1 = 0;
    int indx2 = 0;
    float loopCrossfadeLevel = 0.0f;
};
