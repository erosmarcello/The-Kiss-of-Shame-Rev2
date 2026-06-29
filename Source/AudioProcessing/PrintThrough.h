//
//  PrintThrough.h
//  KissOfShame (Rev 2)
//
//  Finishes the front-panel promise: the PRINT THRU button existed in 2014
//  but was wired to nothing. Adjacent tape wraps magnetize each other; on a
//  tails-out wind this is heard as a soft, dull post-echo that "breathes"
//  under the program.
//
//  Implemented as a single fixed tap (~110 ms) read through a one-pole
//  lowpass, mixed at roughly -22 dB with a smoothed on/off level so the
//  button never clicks. Audible but tape-soft: the rarely-used button now
//  clearly does something. Mirrored by the web demo worklet.
//

#pragma once

#include "../shameConfig.h"

class PrintThrough
{
public:
    void prepare(double newSampleRate, int numChannels)
    {
        sampleRate = newSampleRate;
        delaySamples = jmax(1, (int) std::lround(0.110 * sampleRate));
        delayBuffer.setSize(jmax(1, numChannels), delaySamples);

        lpCoef = (float) jlimit(0.0, 1.0, 5000.0 * MathConstants<double>::twoPi / sampleRate);
        lpState.assign((size_t) jmax(1, numChannels), 0.0f);

        level.reset(sampleRate, 0.05);
        reset();
    }

    void reset()
    {
        delayBuffer.clear();
        writePos = 0;
        std::fill(lpState.begin(), lpState.end(), 0.0f);
        level.setCurrentAndTargetValue(enabled ? echoGain : 0.0f);
    }

    void setEnabled(bool shouldBeEnabled)
    {
        enabled = shouldBeEnabled;
        level.setTargetValue(enabled ? echoGain : 0.0f);
    }

    void process(AudioSampleBuffer& sampleBuffer, int numChannels)
    {
        numChannels = jmin(numChannels, delayBuffer.getNumChannels());

        // Skip entirely once the level has fully ramped out, but keep the
        // delay line warm while ramping so re-engagement is seamless.
        if (! enabled && ! level.isSmoothing())
        {
            for (int i = 0; i < sampleBuffer.getNumSamples(); ++i)
            {
                for (int channel = 0; channel < numChannels; ++channel)
                    delayBuffer.getWritePointer(channel)[writePos] = sampleBuffer.getReadPointer(channel)[i];
                writePos = (writePos + 1) % delaySamples;
            }
            return;
        }

        for (int i = 0; i < sampleBuffer.getNumSamples(); ++i)
        {
            const float gain = level.getNextValue();

            for (int channel = 0; channel < numChannels; ++channel)
            {
                float* samples = sampleBuffer.getWritePointer(channel);
                float* delayed = delayBuffer.getWritePointer(channel);

                const float echo = delayed[writePos]; // oldest sample = full delay
                auto& state = lpState[(size_t) channel];
                state += lpCoef * (echo - state);

                delayed[writePos] = samples[i];
                samples[i] += gain * state;
            }

            writePos = (writePos + 1) % delaySamples;
        }
    }

private:
    double sampleRate = 44100.0;
    int delaySamples = 4851;
    int writePos = 0;

    AudioBuffer<float> delayBuffer;

    float lpCoef = 0.5f;
    std::vector<float> lpState;

    bool enabled = false;
    const float echoGain = 0.08f; // ~ -22 dB
    SmoothedValue<float> level;
};
