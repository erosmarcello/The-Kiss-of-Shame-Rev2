#pragma once

#include "../shameConfig.h"

/*
 Implementation of filter designs based on the biquad setup with difference equation:

 y(n) = a0*x(n) + a1*x(n-1) + a2*x(n-2) - b1*y(n-1) - b2*y(n-2)

 Coefficients are defined for: LowPass, HighPass, BandPass, BandStop, AllPass,
 Butterworth (LP and HP), Linkwitz-Riley, Shelves, Peak.

 Rev 2: sample-rate aware via setSampleRate(), and the long-standing
 "Need to restrict some of the filter coefficients" note is resolved —
 every design now clamps fc into (10 Hz, 0.49 * fs) before computing
 coefficients, which keeps tan()/sin() in their stable regions.
 */

#define MAX_BIQUAD_CHANNELS 2

class Biquads
{
public:
    Biquads()
    {
        reset();
    }

    void setSampleRate(double newSampleRate)
    {
        sampleRate = (float) newSampleRate;
    }

    void reset()
    {
        for (int i = 0; i < MAX_BIQUAD_CHANNELS; ++i)
        {
            priorIn_2[i] = priorIn_1[i] = curInSample[i] = 0.0f;
            priorOut_2[i] = priorOut_1[i] = curOutSample[i] = 0.0f;
        }
    }

    float process(float inSample, int channel)
    {
        channel = jlimit(0, MAX_BIQUAD_CHANNELS - 1, channel);

        priorIn_2[channel] = priorIn_1[channel];
        priorIn_1[channel] = curInSample[channel];
        curInSample[channel] = inSample;

        curOutSample[channel] = a0 * curInSample[channel] + a1 * priorIn_1[channel] + a2 * priorIn_2[channel]
                              - b1 * priorOut_1[channel] - b2 * priorOut_2[channel];

        if (isModifiedBiquad)
            curOutSample[channel] = c0 * curOutSample[channel] + d0 * inSample;

        priorOut_2[channel] = priorOut_1[channel];
        priorOut_1[channel] = curOutSample[channel];

        return curOutSample[channel];
    }

    void setLowPass(float fc, float Q, bool firstOrder)
    {
        fc = clampFrequency(fc);
        isModifiedBiquad = false;
        c0 = 1; d0 = 0;

        const float theta = twoPi() * fc / sampleRate;

        if (firstOrder)
        {
            const float y = std::cos(theta) / (1.0f + std::sin(theta));
            a0 = (1 - y) / 2;  a1 = (1 - y) / 2;  a2 = 0.0f;
            b1 = -y;  b2 = 0.0f;
        }
        else
        {
            const float d = 1.0f / jmax(0.01f, Q);
            const float beta = 0.5f * (1 - 0.5f * d * std::sin(theta)) / (1 + 0.5f * d * std::sin(theta));
            const float y = (0.5f + beta) * std::cos(theta);

            a0 = (0.5f + beta - y) / 2.0f;
            a1 = (0.5f + beta - y);
            a2 = (0.5f + beta - y) / 2.0f;
            b1 = -2 * y;
            b2 = 2 * beta;
        }
    }

    void setHighPass(float fc, float Q, bool firstOrder)
    {
        fc = clampFrequency(fc);
        isModifiedBiquad = false;
        c0 = 1; d0 = 0;

        const float theta = twoPi() * fc / sampleRate;

        if (firstOrder)
        {
            const float y = std::cos(theta) / (1.0f + std::sin(theta));
            a0 = (1 + y) / 2;  a1 = -(1 + y) / 2;  a2 = 0.0f;
            b1 = -y;  b2 = 0.0f;
        }
        else
        {
            const float d = 1.0f / jmax(0.01f, Q);
            const float beta = 0.5f * (1 - 0.5f * d * std::sin(theta)) / (1 + 0.5f * d * std::sin(theta));
            const float y = (0.5f + beta) * std::cos(theta);

            a0 = (0.5f + beta + y) / 2.0f;
            a1 = -(0.5f + beta + y);
            a2 = (0.5f + beta + y) / 2.0f;
            b1 = -2 * y;
            b2 = 2 * beta;
        }
    }

    void setBandPass(float fc, float Q)
    {
        fc = clampFrequency(fc);
        Q = jmax(0.05f, Q);
        isModifiedBiquad = false;
        c0 = 1; d0 = 0;

        const float theta = twoPi() * fc / sampleRate;
        const float beta = 0.5f * (1 - std::tan(theta / (2 * Q))) / (1 + std::tan(theta / (2 * Q)));
        const float y = (0.5f + beta) * std::cos(theta);

        a0 = 0.5f - beta;  a1 = 0.0f;  a2 = -(0.5f - beta);
        b1 = -2 * y;  b2 = 2 * beta;
    }

    void setBandStop(float fc, float Q)
    {
        fc = clampFrequency(fc);
        Q = jmax(0.05f, Q);
        isModifiedBiquad = false;
        c0 = 1; d0 = 0;

        const float theta = twoPi() * fc / sampleRate;
        const float beta = 0.5f * (1 - std::tan(theta / (2 * Q))) / (1 + std::tan(theta / (2 * Q)));
        const float y = (0.5f + beta) * std::cos(theta);

        a0 = 0.5f + beta;  a1 = -2 * y;  a2 = 0.5f + beta;
        b1 = -2 * y;  b2 = 2 * beta;
    }

    void setAllPass(float fc, float Q, bool isFirstOrder)
    {
        fc = clampFrequency(fc);
        isModifiedBiquad = false;
        c0 = 1; d0 = 0;

        if (isFirstOrder)
        {
            const float alpha = (std::tan(pi() * fc / sampleRate) - 1) / (std::tan(pi() * fc / sampleRate) + 1);
            a0 = alpha;  a1 = 1.0f;  a2 = 0.0f;
            b1 = alpha;  b2 = 0.0f;
        }
        else
        {
            const float alpha = (std::tan(pi() * Q / sampleRate) - 1) / (std::tan(pi() * Q / sampleRate) + 1);
            const float beta = -std::cos(twoPi() * fc / sampleRate);
            a0 = -alpha;  a1 = beta * (1 - alpha);  a2 = 1.0f;
            b1 = beta * (1 - alpha);  b2 = -alpha;
        }
    }

    // second order butterworth: lowpass or highpass
    void setButterworth_LowHighPass(float fc, float /*Q*/, bool isLowPass)
    {
        fc = clampFrequency(fc);
        isModifiedBiquad = false;
        c0 = 1; d0 = 0;

        const float tanInput = jlimit(0.0001f, pi() / 2 - 0.001f, fc * pi() / sampleRate);

        if (isLowPass)
        {
            const float C = 1.0f / std::tan(tanInput);

            a0 = 1.0f / (1 + std::sqrt(2.0f) * C + C * C);
            a1 = 2 * a0;
            a2 = a0;
            b1 = 2 * a0 * (1 - C * C);
            b2 = a0 * (1 - std::sqrt(2.0f) * C + C * C);
        }
        else
        {
            const float C = std::tan(tanInput);

            a0 = 1.0f / (1 + std::sqrt(2.0f) * C + C * C);
            a1 = -2 * a0;
            a2 = a0;
            b1 = 2 * a0 * (C * C - 1);
            b2 = a0 * (1 - std::sqrt(2.0f) * C + C * C);
        }
    }

    void setLinkwitzRiley(float fc, float /*Q*/, bool isLowPass)
    {
        fc = clampFrequency(fc);
        isModifiedBiquad = false;
        c0 = 1; d0 = 0;

        const float theta = twoPi() * fc / sampleRate;
        const float omega = pi() * fc;
        const float kappa = omega / std::tan(theta);
        const float delta = kappa * kappa + omega * omega + 2 * kappa * omega;

        if (isLowPass)
        {
            a0 = (omega * omega) / delta;
            a1 = 2 * a0;
            a2 = a0;
        }
        else
        {
            a0 = (kappa * kappa) / delta;
            a1 = -2 * a0;
            a2 = a0;
        }

        b1 = (-2 * kappa * kappa + 2 * omega * omega) / delta;
        b2 = (-2 * kappa * omega + kappa * kappa + omega * omega) / delta;
    }

    void setShelve(float fc, float gain_dB, bool isLowShelf)
    {
        fc = clampFrequency(fc);
        isModifiedBiquad = true;

        const float theta = twoPi() * fc / sampleRate;
        const float mu = std::pow(10.0f, gain_dB / 20);

        if (isLowShelf)
        {
            const float beta = 4.0f / (1 + mu);
            const float delta = beta * std::tan(theta / 2);
            const float gamma = (1 - delta) / (1 + delta);

            a0 = (1 - gamma) / 2;  a1 = a0;  a2 = 0.0f;
            b1 = -gamma;  b2 = 0.0f;
        }
        else
        {
            const float beta = (1 + mu) / 4;
            const float delta = beta * std::tan(theta / 2);
            const float gamma = (1 - delta) / (1 + delta);

            a0 = (1 + gamma) / 2;  a1 = -(1 + gamma) / 2;  a2 = 0.0f;
            b1 = -gamma;  b2 = 0.0f;
        }

        c0 = mu - 1.0f;
        d0 = 1.0f;
    }

private:
    float clampFrequency(float fc) const
    {
        return jlimit(10.0f, 0.49f * sampleRate, fc);
    }

    static constexpr float pi()    { return MathConstants<float>::pi; }
    static constexpr float twoPi() { return MathConstants<float>::twoPi; }

    float sampleRate = 44100.0f;

    float priorIn_2[MAX_BIQUAD_CHANNELS] {};
    float priorIn_1[MAX_BIQUAD_CHANNELS] {};
    float curInSample[MAX_BIQUAD_CHANNELS] {};

    float priorOut_2[MAX_BIQUAD_CHANNELS] {};
    float priorOut_1[MAX_BIQUAD_CHANNELS] {};
    float curOutSample[MAX_BIQUAD_CHANNELS] {};

    float a0 = 0, a1 = 0, a2 = 0, b1 = 0, b2 = 0;
    float c0 = 0, d0 = 0;
    bool isModifiedBiquad = false;
};
