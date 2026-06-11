//
//  InputSaturation.h
//  KissOfShame
//
//  Created by Brian Hansen on 9/9/14.
//  Rev 2: sample-rate aware, allocation-free processing, tape-type voicing.
//
//  The keystone of the plugin: dual-path odd/even harmonic waveshaping
//  (tanh) followed by a one-pole rolloff that models HF loss of the tape
//  path. The original 2014 constants are preserved as the S-111 voicing.
//

#pragma once

#include "../shameConfig.h"

class InputSaturation
{
public:
    InputSaturation(float threshold, float rateOdd, float rateEven)
    {
        satThreshold = jlimit(0.0f, 1.0f, threshold);
        satRateOdd   = jmax(0.0f, rateOdd);
        satRateEven  = jmax(0.0f, rateEven);
    }

    void prepare(double newSampleRate, int maxBlockSize, int numChannels)
    {
        sampleRate = newSampleRate;
        evenBuffer.setSize(jmax(1, numChannels), jmax(1, maxBlockSize), false, true);
        priorSamp.assign((size_t) jmax(1, numChannels), 0.0f);
        setFrequencyRolloff(rolloffHz);
    }

    void reset()
    {
        std::fill(priorSamp.begin(), priorSamp.end(), 0.0f);
    }

    void setRateOdd(float rate)    { satRateOdd = jmax(0.0f, rate); }
    void setRateEven(float rate)   { satRateEven = jmax(0.0f, rate); }
    void setThreshold(float t)     { satThreshold = jlimit(0.0f, 1.0f, t); }
    void setOutput(float o)        { output = o; }

    void setDrive(float normalized)
    {
        // 0..1 mapped over a +/-18dB drive range, like the original.
        drive = Decibels::decibelsToGain(normalized * 36.0f - 18.0f);
    }

    // S-111: the original 1950s-70s formulation — darker, more even-harmonic
    // colour (the untouched 2014 constants).
    // A-456: high-output/low-noise mastering formulation — brighter rolloff,
    // tighter and cleaner saturation.
    void setTapeType(bool useA456)
    {
        if (isA456 == useA456)
            return;

        isA456 = useA456;

        if (isA456)
        {
            evenGain  = 0.15f;
            rolloffHz = 6500.0f;
            satRateOdd = 1.6f;
        }
        else
        {
            evenGain  = 0.3f;
            rolloffHz = 4000.0f;
            satRateOdd = 2.0f;
        }

        setFrequencyRolloff(rolloffHz);
    }

    void setFrequencyRolloff(float f)
    {
        rolloffHz = jmax(0.0f, f);
        coef = (float) (rolloffHz * MathConstants<double>::twoPi / sampleRate);
        coef = jlimit(0.0f, 1.0f, coef);
    }

    void process(AudioSampleBuffer& sampleBuffer, int numChannels)
    {
        const int numSamples = sampleBuffer.getNumSamples();
        numChannels = jmin(numChannels, evenBuffer.getNumChannels());

        for (int ch = 0; ch < numChannels; ++ch)
            evenBuffer.copyFrom(ch, 0, sampleBuffer, ch, 0, numSamples);

        processOddHarmonicWaveshaping(sampleBuffer, numChannels);
        sampleBuffer.applyGain(oddGain);

        processEvenHarmonicWaveshaping(evenBuffer, numChannels, numSamples);
        evenBuffer.applyGain(0, numSamples, evenGain);

        weightEvenAndOddWaveshaping(sampleBuffer, numChannels);

        lowPassFilter(sampleBuffer, numChannels);
    }

private:
    void lowPassFilter(AudioSampleBuffer& sampleBuffer, int numChannels)
    {
        for (int channel = 0; channel < numChannels; ++channel)
        {
            float last = priorSamp[(size_t) channel];
            const float feedback = 1.0f - coef;
            float* sample = sampleBuffer.getWritePointer(channel);

            for (int i = 0; i < sampleBuffer.getNumSamples(); ++i)
                last = sample[i] = coef * sample[i] + feedback * last;

            priorSamp[(size_t) channel] = last;
        }
    }

    void weightEvenAndOddWaveshaping(AudioSampleBuffer& oddBuffer, int numChannels)
    {
        for (int channel = 0; channel < numChannels; ++channel)
        {
            float* odd = oddBuffer.getWritePointer(channel);
            const float* even = evenBuffer.getReadPointer(channel);

            for (int i = 0; i < oddBuffer.getNumSamples(); ++i)
                odd[i] = (oddGain * odd[i] + evenGain * even[i]) / (oddGain + evenGain);
        }
    }

    void processEvenHarmonicWaveshaping(AudioSampleBuffer& buffer, int numChannels, int numSamples)
    {
        for (int channel = 0; channel < numChannels; ++channel)
        {
            float* samples = buffer.getWritePointer(channel);

            for (int i = 0; i < numSamples; ++i)
            {
                samples[i] = std::fabs(samples[i]) * drive;
                samples[i] = std::tanh(satRateEven * samples[i]);
                samples[i] *= output;
            }
        }
    }

    void processOddHarmonicWaveshaping(AudioSampleBuffer& buffer, int numChannels)
    {
        for (int channel = 0; channel < numChannels; ++channel)
        {
            float* samples = buffer.getWritePointer(channel);

            for (int i = 0; i < buffer.getNumSamples(); ++i)
            {
                samples[i] *= drive;

                if (samples[i] > satThreshold)
                    samples[i] = satThreshold + std::tanh(satRateOdd * (std::fabs(samples[i]) - satThreshold)) * (1.0f - satThreshold);
                else if (samples[i] < -satThreshold)
                    samples[i] = -satThreshold - std::tanh(satRateOdd * (std::fabs(samples[i]) - satThreshold)) * (1.0f - satThreshold);

                samples[i] *= output;
            }
        }
    }

    double sampleRate = 44100.0;

    float evenGain = 0.3f;
    float oddGain  = 1.0f;

    float satRateEven  = 0.272f;
    float satRateOdd   = 2.0f;   // how quickly saturation approaches the bound
    float satThreshold = 0.0f;   // level where waveshaping begins

    float drive  = 1.0f;
    float output = 1.0f;

    bool isA456 = false;
    float rolloffHz = 4000.0f;

    AudioSampleBuffer evenBuffer;
    std::vector<float> priorSamp;
    float coef = 0.0f;
};
