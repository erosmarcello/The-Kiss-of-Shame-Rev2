// Offline smoke test for The Kiss of Shame Rev 2.
//
// Boots the real processor (no host, no GUI), drives a sine through the
// default signal path, and verifies the contract Rev 1 never met: audio in,
// audio out — finite, non-silent, and state that round-trips.

#include <PluginProcessor.h>
#include <cmath>
#include <cstdio>

extern juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter();

namespace
{
    constexpr double kSampleRate = 48000.0;
    constexpr int kBlockSize = 512;
    constexpr int kNumBlocks = 200;

    bool bufferIsFinite(const juce::AudioBuffer<float>& buffer)
    {
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            for (int i = 0; i < buffer.getNumSamples(); ++i)
                if (! std::isfinite(buffer.getSample(ch, i)))
                    return false;
        return true;
    }

    int runPassThroughTest(juce::AudioProcessor& proc, const char* label, float shameValue, bool extreme)
    {
        auto& kos = dynamic_cast<KissOfShameAudioProcessor&>(proc);

        if (auto* shame = kos.apvts.getParameter(ParamIDs::shame))
            shame->setValueNotifyingHost(shameValue);
        kos.setShameExtreme(extreme);

        proc.prepareToPlay(kSampleRate, kBlockSize);

        juce::AudioBuffer<float> buffer(2, kBlockSize);
        juce::MidiBuffer midi;

        double phase = 0.0;
        float lastOutRMS = 0.0f;

        for (int b = 0; b < kNumBlocks; ++b)
        {
            for (int i = 0; i < kBlockSize; ++i)
            {
                const float s = 0.5f * (float) std::sin(phase);
                phase += juce::MathConstants<double>::twoPi * 440.0 / kSampleRate;
                buffer.setSample(0, i, s);
                buffer.setSample(1, i, s);
            }

            proc.processBlock(buffer, midi);

            if (! bufferIsFinite(buffer))
            {
                std::printf("FAIL [%s]: non-finite output at block %d\n", label, b);
                return 1;
            }

            lastOutRMS = buffer.getRMSLevel(0, 0, kBlockSize);
        }

        std::printf("  [%s] steady-state output RMS: %f\n", label, lastOutRMS);

        if (lastOutRMS < 0.005f)
        {
            std::printf("FAIL [%s]: output is effectively silent\n", label);
            return 1;
        }
        if (lastOutRMS > 2.0f)
        {
            std::printf("FAIL [%s]: output is blowing up\n", label);
            return 1;
        }

        return 0;
    }
}

int main()
{
    juce::ScopedJuceInitialiser_GUI juceInit;

    std::unique_ptr<juce::AudioProcessor> proc(createPluginFilter());
    proc->setPlayConfigDetails(2, 2, kSampleRate, kBlockSize);

    int failures = 0;

    // 1. The keystone: defaults must pass audio (input 0dB, blend full wet,
    //    output 0dB, everything else at rest).
    failures += runPassThroughTest(*proc, "defaults", 0.0f, false);

    // 2. Shame engaged.
    failures += runPassThroughTest(*proc, "shame=0.7", 0.7f, false);

    // 3. Shame EXTREME (double-click mode).
    failures += runPassThroughTest(*proc, "shame=0.7 EXTREME", 0.7f, true);

    // 4. Every environment at full age must stay finite and audible.
    {
        auto& kos = dynamic_cast<KissOfShameAudioProcessor&>(*proc);
        if (auto* age = kos.apvts.getParameter(ParamIDs::age))
            age->setValueNotifyingHost(1.0f);

        if (auto* env = kos.apvts.getParameter(ParamIDs::environment))
        {
            static const char* names[] = { "Off", "Environs", "StudioCloset", "HumidCellar", "HotLocker", "HurricaneSandy" };
            for (int e = 0; e < 6; ++e)
            {
                env->setValueNotifyingHost(env->convertTo0to1((float) e));
                failures += runPassThroughTest(*proc, names[e], 0.2f, false);
            }
            env->setValueNotifyingHost(0.0f);
        }
    }

    // 5. GUI regression: the Shame cross must actually rotate with the
    //    parameter (a known Rev 1 demo bug), in both eras.
    {
        auto& kos = dynamic_cast<KissOfShameAudioProcessor&>(*proc);
        auto* shame = kos.apvts.getParameter(ParamIDs::shame);

        for (auto eraName : { "heritage", "modern" })
        {
            kos.setUIEra(eraName);

            std::unique_ptr<juce::AudioProcessorEditor> editor(proc->createEditorIfNeeded());
            if (editor == nullptr || shame == nullptr)
            {
                std::printf("FAIL: could not create editor\n");
                ++failures;
                break;
            }

            const juce::Rectangle<int> shameArea(401, 491, 174, 163);

            shame->setValueNotifyingHost(0.0f);
            juce::MessageManager::getInstance()->runDispatchLoopUntil(100);
            auto frameA = editor->createComponentSnapshot(shameArea);

            shame->setValueNotifyingHost(1.0f);
            juce::MessageManager::getInstance()->runDispatchLoopUntil(100);
            auto frameB = editor->createComponentSnapshot(shameArea);

            int64_t diff = 0;
            for (int y = 0; y < frameA.getHeight(); ++y)
                for (int x = 0; x < frameA.getWidth(); ++x)
                {
                    const auto a = frameA.getPixelAt(x, y);
                    const auto b = frameB.getPixelAt(x, y);
                    diff += std::abs((int) a.getRed() - (int) b.getRed())
                          + std::abs((int) a.getGreen() - (int) b.getGreen())
                          + std::abs((int) a.getBlue() - (int) b.getBlue());
                }

            const double meanDiff = (double) diff / (frameA.getWidth() * frameA.getHeight());
            std::printf("  [gui/%s] shame knob mean pixel delta (0 -> 1): %.2f\n", eraName, meanDiff);

            if (meanDiff < 1.0)
            {
                std::printf("FAIL [gui/%s]: the Shame cross does not animate\n", eraName);
                ++failures;
            }

            shame->setValueNotifyingHost(0.0f);
            juce::MessageManager::getInstance()->runDispatchLoopUntil(50);
        }
    }

    // 6. State round-trip: Extreme flag and parameters must survive.
    {
        auto& kos = dynamic_cast<KissOfShameAudioProcessor&>(*proc);
        kos.setShameExtreme(true);
        kos.setUIEra("modern");

        juce::MemoryBlock state;
        proc->getStateInformation(state);

        kos.setShameExtreme(false);
        kos.setUIEra("heritage");
        proc->setStateInformation(state.getData(), (int) state.getSize());

        if (! kos.isShameExtreme() || kos.getUIEra() != "modern")
        {
            std::printf("FAIL: state round-trip lost shameExtreme/uiEra\n");
            ++failures;
        }
        else
        {
            std::printf("  [state] shameExtreme + uiEra survive round-trip\n");
        }
    }

    if (failures == 0)
    {
        std::printf("ALL SMOKE TESTS PASSED\n");
        return 0;
    }

    std::printf("%d SMOKE TEST(S) FAILED\n", failures);
    return 1;
}
