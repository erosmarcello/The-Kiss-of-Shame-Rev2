//
//  Shame.h
//  KissOfShame
//
//  Created by Brian Hansen on 9/9/14.
//  Rev 2: sample-rate aware, cleared buffers, single wavetable, and the
//  Extreme mode — the "all-buttons-in" of the Shame knob.
//
//  Wow/flutter/drift via a modulated playback position over a circular
//  buffer. The knob's three-segment macro mapping is the original 2014
//  curve. Extreme mode (entered by double-clicking the knob, stored as a
//  state property rather than a parameter) drives the same mapping past
//  its design limits and engages a scrape-flutter component.
//

#pragma once

#include "../shameConfig.h"

class Shame
{
public:
    Shame() = default;

    void prepare(double newSampleRate, int numChannels)
    {
        sampleRate = newSampleRate;
        srScale = (float) (sampleRate / 44100.0);

        bufferLength = jmax(1024, (int) std::lround(sampleRate)); // one second of tape
        shameSampleBuffer.setSize(jmax(1, numChannels), bufferLength);
        buildWavetable();
        reset();
    }

    void reset()
    {
        shameSampleBuffer.clear();
        playPosition = 0.0f;
        curPos = 0;
        tablePhase = 0.0f;
        rateFluctuation = 0.0f;
        scrapePhase = 0.0f;
    }

    // The original interpolated macro mapping, with an Extreme overlay.
    void setShame(float input, bool extreme)
    {
        input = jlimit(0.0f, 1.0f, input);
        amount = input;
        extremeActive = extreme;

        if (input <= 0.5f)
        {
            depth = 5.0f * input / 0.5f;
            randPeriodicity = 0.5f;
            rate = 7.0f;
        }
        else if (input <= 0.85f)
        {
            depth = 5.0f + 25.0f * (input - 0.5f) / 0.35f;
            randPeriodicity = 0.5f - 0.25f * (input - 0.5f) / 0.35f;
            rate = 7.0f + 70.0f * (input - 0.5f) / 0.35f;
        }
        else
        {
            depth = 30.0f + 30.0f * (input - 0.85f) / 0.15f;
            randPeriodicity = 0.25f + 0.5f * (input - 0.85f) / 0.15f;
            rate = 77.0f - 20.0f * (input - 0.85f) / 0.15f;
        }

        if (extremeActive)
        {
            // Past the stops: deeper excursions, faster and less periodic
            // modulation, plus scrape-flutter that no front-panel setting
            // could reach.
            depth *= 2.5f;
            rate = jmin(rate * 1.6f, 120.0f);
            randPeriodicity = jmin(1.0f, randPeriodicity * 1.5f + 0.15f);
            scrapeDepth = 4.0f * input * srScale;
            scrapeRateHz = 38.0f + 30.0f * input;
        }
        else
        {
            scrapeDepth = 0.0f;
        }
    }

    void process(AudioSampleBuffer& sampleBuffer, int numChannels)
    {
        numChannels = jmin(numChannels, shameSampleBuffer.getNumChannels());
        const float depthSamples = depth * srScale;

        for (int i = 0; i < sampleBuffer.getNumSamples(); ++i)
        {
            for (int channel = 0; channel < numChannels; ++channel)
                shameSampleBuffer.getWritePointer(channel)[curPos] = sampleBuffer.getReadPointer(channel)[i];

            const float frac = playPosition - (float) (int) playPosition;
            const int prX = (int) playPosition;
            const int nxtX = (prX + 1) % bufferLength;

            for (int channel = 0; channel < numChannels; ++channel)
            {
                const float* tape = shameSampleBuffer.getReadPointer(channel);
                sampleBuffer.getWritePointer(channel)[i] = tape[prX] * (1.0f - frac) + tape[nxtX] * frac;
            }

            playPosition = (float) curPos + depthSamples * processWavetable();

            if (extremeActive && scrapeDepth > 0.0f)
            {
                playPosition += scrapeDepth * std::sin(scrapePhase);
                scrapePhase += (float) (MathConstants<double>::twoPi * scrapeRateHz / sampleRate);
                if (scrapePhase > MathConstants<float>::twoPi)
                    scrapePhase -= MathConstants<float>::twoPi;
            }

            while (playPosition >= (float) bufferLength) playPosition -= (float) bufferLength;
            while (playPosition < 0.0f)                  playPosition += (float) bufferLength;

            curPos = (curPos + 1) % bufferLength;
        }
    }

private:
    void buildWavetable()
    {
        for (int j = 0; j < tableSize; ++j)
        {
            const double phase = MathConstants<double>::twoPi * (double) j / (double) (tableSize - 1);
            wavetable[(size_t) j] = (float) (0.5 * (std::cos(phase) - 1.0));
        }
    }

    float processWavetable()
    {
        const float fracPos = tablePhase - (float) (int) tablePhase;
        const int prPos = (int) tablePhase;
        const int nxtPos = (prPos + 1) % tableSize;

        const float outsample = wavetable[(size_t) prPos] * (1.0f - fracPos) + wavetable[(size_t) nxtPos] * fracPos;

        // rate is in Hz: full table traversal per 1/rate seconds.
        tablePhase += (rate + rateFluctuation) * (float) tableSize / (float) sampleRate;

        if (tablePhase >= (float) tableSize)
        {
            // Each cycle picks a new random rate deviation, scaled by the
            // periodicity — the stochastic heart of the wow/flutter.
            rateFluctuation = (random.nextFloat() * 2.0f - 1.0f) * rate * randPeriodicity;
            tablePhase -= (float) tableSize;
        }
        if (tablePhase < 0.0f)
            tablePhase += (float) tableSize;

        return outsample;
    }

    double sampleRate = 44100.0;
    float srScale = 1.0f;

    AudioSampleBuffer shameSampleBuffer;
    int bufferLength = 44100;

    float playPosition = 0.0f;
    int curPos = 0;

    float amount = 0.0f;
    float depth = 0.0f;
    float rate = 7.0f;
    float randPeriodicity = 0.5f;
    float rateFluctuation = 0.0f;

    bool extremeActive = false;
    float scrapeDepth = 0.0f;
    float scrapeRateHz = 38.0f;
    float scrapePhase = 0.0f;

    static constexpr int tableSize = 8192;
    std::array<float, tableSize> wavetable {};
    float tablePhase = 0.0f;

    Random random;
};
