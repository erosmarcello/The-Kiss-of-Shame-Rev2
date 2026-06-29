//
//  Flange.h
//  KissOfShame
//
//  Created by Brian Hansen on 9/9/14.
//  Rev 2: rebuilt as an actual flanger. Dragging the animated reels leans on
//  the tape; the drag depth (0..1) drives a short LFO-swept delay with
//  feedback. At rest the stage is fully dry (transparent). As you lean in, the
//  wet mix, sweep depth, feedback resonance and LFO rate all grow, giving the
//  classic sweeping comb. Mirrored sample-for-sample by the web demo worklet.
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
        const float sr = (float) sampleRate;

        bufferLength = jmax(256, (int) std::lround(0.05 * sampleRate)); // 50 ms line
        flangeSampleBuffer.setSize(jmax(1, numChannels), bufferLength);

        baseDelay  = 0.0008f * sr;  // 0.8 ms base delay
        sweepDepth = 0.0045f * sr;  // up to 4.5 ms LFO sweep
        intSmooth  = 1.0f - std::exp(-1.0f / (0.010f * sr)); // ~10 ms intensity glide

        reset();
    }

    void reset()
    {
        flangeSampleBuffer.clear();
        writePos = 0;
        lfoPhase = 0.0f;
        curIntensity = targetIntensity;
    }

    // Input 0..1: how hard the reels are leaned on.
    void setDepth(float depth01)
    {
        targetIntensity = jlimit(0.0f, 1.0f, depth01);
    }

    void process(AudioSampleBuffer& sampleBuffer, int numChannels)
    {
        numChannels = jmin(numChannels, flangeSampleBuffer.getNumChannels());
        const float sr = (float) sampleRate;
        const int L = bufferLength;

        for (int i = 0; i < sampleBuffer.getNumSamples(); ++i)
        {
            curIntensity += (targetIntensity - curIntensity) * intSmooth;
            const float fi  = curIntensity;
            const float wet = 0.5f * jmin(1.0f, fi * 4.0f); // dry at rest, fades in fast
            const float fb  = 0.65f * fi;                   // resonance grows with lean
            const float lfoRate = 0.25f + 0.35f * fi;

            lfoPhase += lfoRate / sr;
            if (lfoPhase >= 1.0f) lfoPhase -= 1.0f;
            const float lfo = 0.5f * (1.0f - std::cos(MathConstants<float>::twoPi * lfoPhase));

            float readPos = (float) writePos - (baseDelay + sweepDepth * fi * lfo);
            while (readPos >= (float) L) readPos -= (float) L;
            while (readPos < 0.0f)       readPos += (float) L;

            const int i0 = (int) readPos;
            const int i1 = (i0 + 1) % L;
            const float frac = readPos - (float) i0;

            for (int channel = 0; channel < numChannels; ++channel)
            {
                float* samples = sampleBuffer.getWritePointer(channel);
                float* line    = flangeSampleBuffer.getWritePointer(channel);

                const float delayed = line[i0] * (1.0f - frac) + line[i1] * frac;
                const float in = samples[i];
                line[writePos] = in + fb * delayed;            // feedback into the line
                samples[i] = (1.0f - wet) * in + wet * delayed; // mix (dry when wet == 0)
            }

            writePos = (writePos + 1) % L;
        }
    }

private:
    double sampleRate = 44100.0;

    AudioSampleBuffer flangeSampleBuffer;
    int bufferLength = 2205;
    int writePos = 0;

    float baseDelay = 35.0f;
    float sweepDepth = 198.0f;
    float intSmooth = 0.004f;

    float lfoPhase = 0.0f;
    float curIntensity = 0.0f;
    float targetIntensity = 0.0f;
};
