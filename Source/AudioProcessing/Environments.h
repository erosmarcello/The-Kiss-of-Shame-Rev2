//
//  Environments.h
//  KissOfShame (Rev 2)
//
//  The storage-environment engine, rebuilt from the ground up. Rev 1
//  implemented only Hurricane Sandy; the other selector positions were
//  silent. Rev 2 gives every position a distinct sonic fingerprint, all
//  composed from the same primitives (dips, grains, loops, biquads) and
//  all driven by the AGE knob as the single intensity input — the
//  "environmental storytelling" model: pick where the reel was stored,
//  then dial in how long it suffered there.
//
//      Studio Closet   — mild oxidation: dulling, light dropouts, rare crackle
//      Humid Cellar    — sticky-shed: heavy muffling, slow undulation, damp rumble
//      Hot Locker      — heat warp: slow wow, sagging level, exaggerated print-through
//      Hurricane Sandy — the flood-damaged reel (original 2014 recipe, debugged)
//

#pragma once

#include "../shameConfig.h"
#include "../ShameResources.h"
#include "EnvelopeDips.h"
#include "Granulate.h"
#include "LoopCrossfade.h"
#include "Biquads.h"
#include "Envelope.h"
#include "Noise.h"

//==============================================================================
class EnvironmentFX
{
public:
    virtual ~EnvironmentFX() = default;

    virtual void prepare(double sampleRate, const ShameResources::AudioAssets& assets) = 0;
    virtual void reset() {}
    virtual void setIntensity(float intensity01) = 0;
    virtual void process(AudioSampleBuffer& buffer, int numChannels) = 0;
};

//==============================================================================
// Mild oxidation in dry darkness: dulled top end, light dropouts, and the
// occasional crackle of a flaking oxide particle passing the head.
class StudioClosetFX : public EnvironmentFX
{
public:
    void prepare(double sampleRate, const ShameResources::AudioAssets&) override
    {
        sr = sampleRate;
        lp.setSampleRate(sampleRate);
        lp.reset();
        crackleHP.setSampleRate(sampleRate);
        crackleHP.setButterworth_LowHighPass(1200.0f, 1.0f, false);
        crackleHP.reset();
        dips.prepare(sampleRate);
        dips.setDomainMS(900);
        dips.setNumPoints(12);
        dips.setNumPointRandomness(0.5f);
        crackleEnv = 0.0f;
        setIntensity(0.0f);
    }

    void reset() override { lp.reset(); crackleHP.reset(); crackleEnv = 0.0f; }

    void setIntensity(float v) override
    {
        intensity = jlimit(0.0f, 1.0f, v);
        lp.setButterworth_LowHighPass(14000.0f * (1.0f - intensity) + 6500.0f * intensity, 1.0f, true);
        dips.setDynamicExtremity(0.25f * intensity);
        // a few crackles per second at full age
        crackleProbability = 6.0f * intensity / (float) sr;
    }

    void process(AudioSampleBuffer& buffer, int numChannels) override
    {
        if (intensity <= 0.0001f)
            return;

        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            const float dipLevel = dips.processEnvelopeDips();

            if (random.nextFloat() < crackleProbability)
                crackleEnv = 0.12f * intensity * (0.4f + 0.6f * random.nextFloat());
            crackleEnv *= 0.992f;

            const float crackle = crackleHP.process(crackleEnv * noise.tick(), 0);

            for (int ch = 0; ch < numChannels; ++ch)
            {
                float* s = buffer.getWritePointer(ch);
                if (ch < MAX_BIQUAD_CHANNELS)
                    s[i] = lp.process(s[i], ch);
                s[i] *= (1.0f - intensity) + intensity * dipLevel;
                s[i] += crackle;
            }
        }
    }

private:
    double sr = 44100.0;
    float intensity = 0.0f;
    float crackleProbability = 0.0f;
    float crackleEnv = 0.0f;

    Biquads lp, crackleHP;
    EnvelopeDips dips;
    Noise noise;
    Random random;
};

//==============================================================================
// Sticky-shed syndrome: binder absorbs moisture, the top end drowns, the
// level undulates slowly, and a damp rumble rides underneath.
class HumidCellarFX : public EnvironmentFX
{
public:
    void prepare(double sampleRate, const ShameResources::AudioAssets& assets) override
    {
        lp.setSampleRate(sampleRate);
        lp.reset();
        rumbleLP.setSampleRate(sampleRate);
        rumbleLP.setButterworth_LowHighPass(220.0f, 1.0f, true);
        rumbleLP.reset();

        dips.prepare(sampleRate);
        dips.setDomainMS(2500);
        dips.setNumPoints(6);
        dips.setNumPointRandomness(0.6f);

        rumbleBed.setAudioData(assets.lowLevelGrainNoise);
        setIntensity(0.0f);
    }

    void reset() override { lp.reset(); rumbleLP.reset(); rumbleBed.reset(); }

    void setIntensity(float v) override
    {
        intensity = jlimit(0.0f, 1.0f, v);
        lp.setButterworth_LowHighPass(10000.0f * (1.0f - intensity) + 2800.0f * intensity, 1.0f, true);
        dips.setDynamicExtremity(0.45f * intensity);
        rumbleBed.setLoopCrossfadeLevel(1.0f);
        rumbleLevel = 0.1f * intensity;
    }

    void process(AudioSampleBuffer& buffer, int numChannels) override
    {
        if (intensity <= 0.0001f)
            return;

        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            const float dipLevel = dips.processEnvelopeDips();
            const float rumble = rumbleLevel * rumbleLP.process(rumbleBed.processLoopCrossSample(0), 0);

            for (int ch = 0; ch < numChannels; ++ch)
            {
                float* s = buffer.getWritePointer(ch);
                if (ch < MAX_BIQUAD_CHANNELS)
                    s[i] = lp.process(s[i], ch);
                s[i] *= (1.0f - intensity) + intensity * dipLevel;
                s[i] += rumble;
            }
        }
    }

private:
    float intensity = 0.0f;
    float rumbleLevel = 0.0f;

    Biquads lp, rumbleLP;
    EnvelopeDips dips;
    LoopCrossfade rumbleBed;
};

//==============================================================================
// Heat warp: the pack deforms, speed sags slowly (deep wow), the level
// breathes, and layer-to-layer magnetization prints harder than it should.
class HotLockerFX : public EnvironmentFX
{
public:
    void prepare(double sampleRate, const ShameResources::AudioAssets&) override
    {
        sr = sampleRate;
        srScale = (float) (sr / 44100.0);

        wowBufferLength = jmax(256, (int) std::lround(4096.0 * srScale));
        wowBuffer.setSize(2, wowBufferLength);
        printDelaySamples = jmax(1, (int) std::lround(0.090 * sr));
        printBuffer.setSize(2, printDelaySamples);

        lp.setSampleRate(sampleRate);
        lp.reset();
        dips.prepare(sampleRate);
        dips.setDomainMS(800);
        dips.setNumPoints(10);
        dips.setNumPointRandomness(0.4f);

        reset();
        setIntensity(0.0f);
    }

    void reset() override
    {
        wowBuffer.clear();
        printBuffer.clear();
        wowWritePos = 0;
        printWritePos = 0;
        wowPhase = 0.0f;
        lp.reset();
    }

    void setIntensity(float v) override
    {
        intensity = jlimit(0.0f, 1.0f, v);
        lp.setButterworth_LowHighPass(12000.0f * (1.0f - intensity) + 6000.0f * intensity, 1.0f, true);
        dips.setDynamicExtremity(0.3f * intensity);
        wowDepthSamples = 28.0f * intensity * srScale;
        wowRateHz = 0.6f + 0.7f * intensity;
        printLevel = Decibels::decibelsToGain(-38.0f + 12.0f * intensity); // up to -26 dB
    }

    void process(AudioSampleBuffer& buffer, int numChannels) override
    {
        if (intensity <= 0.0001f)
            return;

        numChannels = jmin(numChannels, 2);

        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            const float dipLevel = dips.processEnvelopeDips();

            // slow wow via a modulated delay read
            const float mod = 0.5f * (1.0f - std::cos(wowPhase)); // 0..1
            wowPhase += (float) (MathConstants<double>::twoPi * wowRateHz / sr);
            if (wowPhase > MathConstants<float>::twoPi)
                wowPhase -= MathConstants<float>::twoPi;

            float readPos = (float) wowWritePos - 1.0f - wowDepthSamples * mod;
            while (readPos < 0.0f) readPos += (float) wowBufferLength;
            const int prX = (int) readPos;
            const int nxtX = (prX + 1) % wowBufferLength;
            const float frac = readPos - (float) prX;

            for (int ch = 0; ch < numChannels; ++ch)
            {
                float* s = buffer.getWritePointer(ch);

                wowBuffer.getWritePointer(ch)[wowWritePos] = s[i];
                const float* tape = wowBuffer.getReadPointer(ch);
                float warped = tape[prX] * (1.0f - frac) + tape[nxtX] * frac;

                // exaggerated print-through
                float* print = printBuffer.getWritePointer(ch);
                const float echo = print[printWritePos];
                print[printWritePos] = warped;
                warped += printLevel * echo;

                if (ch < MAX_BIQUAD_CHANNELS)
                    warped = lp.process(warped, ch);

                s[i] = warped * ((1.0f - intensity) + intensity * dipLevel);
            }

            wowWritePos = (wowWritePos + 1) % wowBufferLength;
            printWritePos = (printWritePos + 1) % printDelaySamples;
        }
    }

private:
    double sr = 44100.0;
    float srScale = 1.0f;
    float intensity = 0.0f;

    AudioBuffer<float> wowBuffer;
    int wowBufferLength = 4096;
    int wowWritePos = 0;
    float wowPhase = 0.0f;
    float wowDepthSamples = 0.0f;
    float wowRateHz = 0.6f;

    AudioBuffer<float> printBuffer;
    int printDelaySamples = 3969;
    int printWritePos = 0;
    float printLevel = 0.0f;

    Biquads lp;
    EnvelopeDips dips;
};

//==============================================================================
// The flood-damaged reel: the original 2014 recipe — periodic noise bursts,
// pink-noise granulation, amplitude dips, low-frequency grain bed and a
// sweeping lowpass. Rev 2 fixes the uninitialized burst level (it was only
// assigned above half intensity) and feeds it embedded, rate-matched audio.
class HurricaneSandyFX : public EnvironmentFX
{
public:
    void prepare(double sampleRate, const ShameResources::AudioAssets& assets) override
    {
        dips.prepare(sampleRate);
        dips.setDomainMS(1000);
        dips.setDynamicExtremity(0.5f);
        dips.setNumPoints(15);
        dips.setNumPointRandomness(0.5f);
        dips.calculateDipPoints();

        noiseEnv = std::make_unique<Envelope>(350);
        noiseEnv->addEvelopePoint(0.143f, 0.073f);
        noiseEnv->addEvelopePoint(0.305f, 0.367f);
        noiseEnv->addEvelopePoint(0.383f, 0.567f);
        noiseEnv->addEvelopePoint(0.428f, 0.0f);
        noiseEnv->prepare(sampleRate);

        sigEnv = std::make_unique<Envelope>(350);
        sigEnv->addEvelopePoint(0.143f, 0.2f);
        sigEnv->addEvelopePoint(0.305f, 0.8f);
        sigEnv->addEvelopePoint(0.383f, 0.5f);
        sigEnv->addEvelopePoint(0.428f, 0.0f);
        sigEnv->prepare(sampleRate);

        granulator.setSampleRate(sampleRate);
        granulator.setAudioData(assets.pinkNoise);
        granulator.setRandomFactor(1.0f);
        granulator.setStretch(0);
        granulator.setGrainParameters(5, 50, 50, 50);
        granulator.setVoices(10);

        lowFreqGranular.setAudioData(assets.lowLevelGrainNoise);
        lowFreqGranular.setLoopCrossfadeLevel(0.25f);

        lpButterworth_Grains.setSampleRate(sampleRate);
        lpButterworth_Grains.setButterworth_LowHighPass(2000.0f, 1.0f, true);
        hpButterworth_Grains.setSampleRate(sampleRate);
        hpButterworth_Grains.setButterworth_LowHighPass(50.0f, 1.0f, false);
        lpButterworth_Signal.setSampleRate(sampleRate);
        lpButterworth_Signal.setButterworth_LowHighPass(22050.0f, 1.0f, true);

        setIntensity(0.0f);
    }

    void reset() override
    {
        lpButterworth_Grains.reset();
        hpButterworth_Grains.reset();
        lpButterworth_Signal.reset();
        lowFreqGranular.reset();
        granulator.reset();
    }

    void setIntensity(float input) override
    {
        input = jlimit(0.0f, 1.0f, input);
        intensity = input;

        // grain interpolation
        grainImpact = input;
        granulator.setGrainParameters((unsigned int) (10 * input + 5), 75, 50, (unsigned int) (1000 * (1.01f - input)));

        // amount of low frequency granular noise mixed in.
        lowFreqGrainNoiseLevel = 0.15f * input;

        // amplitude fluctuation
        ampFluctuationImpact = input;

        // periodic noise bursts only engage past half intensity
        noiseBurstImpact = input > 0.5f ? 2.0f * (input - 0.5f) : 0.0f;

        // low pass filtering of original signal
        lpButterworth_Signal.setButterworth_LowHighPass(20050.0f * (1.0f - input) + 2000.0f, 1.0f, true);
    }

    void process(AudioSampleBuffer& buffer, int numChannels) override
    {
        if (intensity <= 0.0001f)
            return;

        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            const float lfGrainSample = lowFreqGranular.processLoopCrossSample(0);
            float grainSample = granulator.tick();

            grainSample = lpButterworth_Grains.process(grainSample, 0);
            grainSample = hpButterworth_Grains.process(grainSample, 0);

            const float dipsLevel = dips.processEnvelopeDips();
            const float noiseBurstEnvValue = noiseEnv->processEnvelope();
            const float signalEnvValue = 1.0f - sigEnv->processEnvelope();
            const float burstNoise = whiteNoise.tick();

            for (int channel = 0; channel < numChannels; ++channel)
            {
                float* samples = buffer.getWritePointer(channel);

                // lowpass filter the original signal
                if (channel < MAX_BIQUAD_CHANNELS)
                    samples[i] = lpButterworth_Signal.process(samples[i], channel);

                // mix signal with periodic noise burst.
                samples[i] = (1.0f - noiseBurstImpact) * samples[i]
                           + noiseBurstImpact * (signalEnvValue * samples[i] + 0.05f * noiseBurstEnvValue * burstNoise);

                // modulate the amplitude by the granular amplitude
                samples[i] *= (1.0f - grainImpact * grainSample);

                // fluctuate the overall amplitude
                samples[i] = ampFluctuationImpact * dipsLevel * samples[i] + (1.0f - ampFluctuationImpact) * samples[i];

                // mix in the low freq grain noise bed.
                samples[i] += lowFreqGrainNoiseLevel * lfGrainSample;
            }
        }
    }

private:
    float intensity = 0.0f;

    EnvelopeDips dips;
    Granulate granulator;
    LoopCrossfade lowFreqGranular;
    Biquads lpButterworth_Grains;
    Biquads hpButterworth_Grains;
    Biquads lpButterworth_Signal;

    Noise whiteNoise;
    std::unique_ptr<Envelope> noiseEnv;
    std::unique_ptr<Envelope> sigEnv;

    float grainImpact = 0.0f;
    float lowFreqGrainNoiseLevel = 0.0f;
    float ampFluctuationImpact = 0.0f;
    float noiseBurstImpact = 0.0f;
};
