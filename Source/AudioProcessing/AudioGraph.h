//
//  AudioGraph.h
//  KissOfShame
//
//  Created by Brian Hansen on 9/9/14.
//  Rev 2: prepared/allocation-free processing, click-free bypass, all five
//  environments live, tape-type voicing, working print-through, and the
//  Shame Extreme overlay. Signal flow preserved from the original design:
//
//      input drive -> [saturation -> flange -> environment -> hiss ->
//      shame -> print-through] -> blend -> output level
//

#pragma once

#include "../shameConfig.h"
#include "../ShameResources.h"
#include "Hiss.h"
#include "Shame.h"
#include "InputSaturation.h"
#include "Blend.h"
#include "Flange.h"
#include "PrintThrough.h"
#include "Environments.h"

class AudioGraph
{
public:
    AudioGraph()
        : inSaturation(0.0f, 2.0f, 0.272f)
    {
        environments[eEnvironmentOff]            = nullptr;
        environments[eEnvironmentStudioCloset]   = std::make_unique<StudioClosetFX>();
        environments[eEnvironmentHumidCellar]    = std::make_unique<HumidCellarFX>();
        environments[eEnvironmentHotLocker]      = std::make_unique<HotLockerFX>();
        environments[eEnvironmentHurricaneSandy] = std::make_unique<HurricaneSandyFX>();
    }

    void prepare(double sampleRate, int maxBlockSize, int numChannels)
    {
        preparedChannels = jmax(1, numChannels);

        assets.prepare(sampleRate);

        inSaturation.prepare(sampleRate, maxBlockSize, preparedChannels);
        flange.prepare(sampleRate, preparedChannels);
        shame.prepare(sampleRate, preparedChannels);
        hiss.setAudioData(assets.hiss);
        printThrough.prepare(sampleRate, preparedChannels);

        for (auto& env : environments)
            if (env != nullptr)
                env->prepare(sampleRate, assets);

        processingBuffer.setSize(preparedChannels, jmax(1, maxBlockSize));
        dryBuffer.setSize(preparedChannels, jmax(1, maxBlockSize));

        bypassMix.reset(sampleRate, 0.05);
        bypassMix.setCurrentAndTargetValue(bypassGraph ? 1.0f : 0.0f);

        applyAge();
    }

    void reset()
    {
        inSaturation.reset();
        flange.reset();
        shame.reset();
        hiss.reset();
        printThrough.reset();
        for (auto& env : environments)
            if (env != nullptr)
                env->reset();
    }

    //==========================================================================
    void setCurrentEnvironment(EShameEnvironments env)
    {
        if (currentEnvironment == env)
            return;

        currentEnvironment = env;

        if (auto* fx = activeEnvironment())
        {
            fx->reset();
            fx->setIntensity(age);
        }
    }

    void setAge(float newAge)
    {
        if (std::abs(newAge - age) < 1.0e-6f)
            return;

        age = jlimit(0.0f, 1.0f, newAge);
        applyAge();
    }

    void setShame(float amount, bool extreme)
    {
        if (std::abs(amount - shameAmount) < 1.0e-6f && extreme == shameExtreme)
            return;

        shameAmount = amount;
        shameExtreme = extreme;
        shame.setShame(shameAmount, shameExtreme);
    }

    void setInputDrive(float drive01)
    {
        if (std::abs(drive01 - inputDrive01) < 1.0e-6f)
            return;

        inputDrive01 = drive01;
        inputDrive = Decibels::decibelsToGain(drive01 * 36.0f - 18.0f);
    }

    void setOutputLevel(float level01)
    {
        if (std::abs(level01 - outputLevel01) < 1.0e-6f)
            return;

        outputLevel01 = level01;
        outputLevel = Decibels::decibelsToGain(level01 * 36.0f - 18.0f);
    }

    void setHissLevel(float level)       { hiss.setHissLevel(level); }
    void setBlendLevel(float level)      { blend.setBlendLevel(level); }
    void setFlangeDepth(float depth01)   { flange.setDepth(depth01); }

    void setTapeType(bool useA456)
    {
        inSaturation.setTapeType(useA456);
        hiss.setTapeType(useA456);
    }

    void setPrintThrough(bool enabled)   { printThrough.setEnabled(enabled); }

    void setBypass(bool shouldBypass)
    {
        if (bypassGraph == shouldBypass)
            return;

        bypassGraph = shouldBypass;
        bypassMix.setTargetValue(bypassGraph ? 1.0f : 0.0f);
    }

    bool isGraphBypassed() const         { return bypassGraph; }
    float getOutputLevel() const         { return outputLevel; }

    //==========================================================================
    void processGraph(AudioSampleBuffer& audioBuffer, int numChannels)
    {
        const int numSamples = audioBuffer.getNumSamples();
        numChannels = jmin(numChannels, preparedChannels);

        // Fully bypassed and done ramping: true bypass, zero work.
        if (bypassGraph && ! bypassMix.isSmoothing())
            return;

        const bool ramping = bypassMix.isSmoothing();
        if (ramping)
            for (int ch = 0; ch < numChannels; ++ch)
                dryBuffer.copyFrom(ch, 0, audioBuffer, ch, 0, numSamples);

        // apply the input drive
        audioBuffer.applyGain(inputDrive);

        // copy of the driven signal, used strictly for wet processing
        for (int ch = 0; ch < numChannels; ++ch)
            processingBuffer.copyFrom(ch, 0, audioBuffer, ch, 0, numSamples);

        // Trick: the wet path operates on a sub-view sized to this block.
        AudioSampleBuffer wetBlock(processingBuffer.getArrayOfWritePointers(), numChannels, numSamples);

        // 1. incoming audio gets saturation and flange processing
        inSaturation.process(wetBlock, numChannels);
        flange.process(wetBlock, numChannels);

        // 2. storage environment
        if (auto* fx = activeEnvironment())
            fx->process(wetBlock, numChannels);

        // 3. hiss and the shame feature
        hiss.process(wetBlock, numChannels);
        shame.process(wetBlock, numChannels);

        // 4. layer-to-layer print-through
        printThrough.process(wetBlock, numChannels);

        // 5. blend processed audio with the (driven) original signal
        blend.processBlend(audioBuffer, processingBuffer, numChannels);

        // 6. final output level
        audioBuffer.applyGain(outputLevel);

        // 7. click-free bypass crossfade back to the untouched input
        if (ramping)
        {
            for (int i = 0; i < numSamples; ++i)
            {
                const float mix = bypassMix.getNextValue();
                for (int ch = 0; ch < numChannels; ++ch)
                {
                    float* out = audioBuffer.getWritePointer(ch);
                    out[i] = (1.0f - mix) * out[i] + mix * dryBuffer.getReadPointer(ch)[i];
                }
            }
        }
    }

private:
    EnvironmentFX* activeEnvironment() const
    {
        if (currentEnvironment > eEnvironmentOff && currentEnvironment < eEnvironmentTotalEnvironments)
            return environments[currentEnvironment].get();
        return nullptr;
    }

    void applyAge()
    {
        for (auto& env : environments)
            if (env != nullptr)
                env->setIntensity(age);
    }

    EShameEnvironments currentEnvironment = eEnvironmentOff;

    ShameResources::AudioAssets assets;

    InputSaturation inSaturation;
    Flange flange;
    Shame shame;
    Hiss hiss;
    Blend blend;
    PrintThrough printThrough;

    std::array<std::unique_ptr<EnvironmentFX>, eEnvironmentTotalEnvironments> environments;

    AudioSampleBuffer processingBuffer;
    AudioSampleBuffer dryBuffer;

    int preparedChannels = 2;

    bool bypassGraph = false;
    SmoothedValue<float> bypassMix;

    float age = 0.0f;
    float shameAmount = 0.0f;
    bool shameExtreme = false;

    float inputDrive01 = 0.5f;
    float outputLevel01 = 0.5f;
    float inputDrive = 1.0f;
    float outputLevel = 1.0f;
};
