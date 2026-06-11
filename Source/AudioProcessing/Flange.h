//
//  Flange.h
//  KissOfShame
//
//  Created by Brian Hansen on 9/9/14.
//  Rev 2: sample-rate aware buffer and depth scaling.
//
//  The reel-touch flange: dragging the animated reels modulates the depth,
//  reading behind the write head through a smoothed variable delay.
//

#pragma once

#include "../shameConfig.h"

class Flange
{
public:
    Flange() = default;

    void prepare(double newSampleRate, int numChannels)
    {
        sampleRate = newSampleRate;
        srScale = (float) (sampleRate / 44100.0);

        bufferLength = jmax(64, (int) std::lround(2000.0 * srScale));
        flangeSampleBuffer.setSize(jmax(1, numChannels), bufferLength);

        // The original smoothing moved 0.1% of the remaining distance per
        // sample at 44.1k; keep that time constant at any rate.
        smoothingCoeff = jlimit(0.00001f, 1.0f, 0.001f / srScale);
        snapThreshold = 0.01f * srScale;

        reset();
    }

    void reset()
    {
        flangeSampleBuffer.clear();
        playPosition = 0.0f;
        curPos = 0;
        curDepth = 0.0f;
    }

    // Input 0..1; the original mapped this across 1000 samples of delay.
    void setDepth(float depth01)
    {
        targetDepth = depth01 * 1000.0f * srScale;
    }

    void process(AudioSampleBuffer& sampleBuffer, int numChannels)
    {
        numChannels = jmin(numChannels, flangeSampleBuffer.getNumChannels());

        for (int i = 0; i < sampleBuffer.getNumSamples(); ++i)
        {
            for (int channel = 0; channel < numChannels; ++channel)
                flangeSampleBuffer.getWritePointer(channel)[curPos] = sampleBuffer.getReadPointer(channel)[i];

            const float frac = playPosition - (float) (int) playPosition;
            const int prX = (int) playPosition;
            const int nxtX = (prX + 1) % bufferLength;

            for (int channel = 0; channel < numChannels; ++channel)
            {
                float* out = sampleBuffer.getWritePointer(channel);
                const float* delayed = flangeSampleBuffer.getReadPointer(channel);
                out[i] = 0.5f * out[i] + 0.5f * (delayed[prX] * (1.0f - frac) + delayed[nxtX] * frac);
            }

            if (std::fabs(targetDepth - curDepth) < snapThreshold)
                curDepth = targetDepth;
            else
                curDepth += (targetDepth - curDepth) * smoothingCoeff;

            playPosition = (float) curPos - curDepth;

            while (playPosition >= (float) bufferLength) playPosition -= (float) bufferLength;
            while (playPosition < 0.0f)                  playPosition += (float) bufferLength;

            curPos = (curPos + 1) % bufferLength;
        }
    }

private:
    double sampleRate = 44100.0;
    float srScale = 1.0f;

    AudioSampleBuffer flangeSampleBuffer;
    int bufferLength = 2000;

    float playPosition = 0.0f;
    int curPos = 0;

    float curDepth = 0.0f;
    float targetDepth = 0.0f;
    float smoothingCoeff = 0.001f;
    float snapThreshold = 0.01f;
};
