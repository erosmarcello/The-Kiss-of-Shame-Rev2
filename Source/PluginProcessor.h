#pragma once

#include <JuceHeader.h>
#include "AudioProcessing/AudioGraph.h"
#include "Parameters.h"

//==============================================================================
class KissOfShameAudioProcessor : public AudioProcessor
{
public:
    KissOfShameAudioProcessor();
    ~KissOfShameAudioProcessor() override = default;

    //==========================================================================
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(AudioBuffer<float>&, MidiBuffer&) override;

    //==========================================================================
    AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override                      { return true; }

    const String getName() const override                { return JucePlugin_Name; }
    bool acceptsMidi() const override                    { return false; }
    bool producesMidi() const override                   { return false; }
    bool isMidiEffect() const override                   { return false; }
    double getTailLengthSeconds() const override         { return 0.0; }

    int getNumPrograms() override                        { return 1; }
    int getCurrentProgram() override                     { return 0; }
    void setCurrentProgram(int) override                 {}
    const String getProgramName(int) override            { return {}; }
    void changeProgramName(int, const String&) override  {}

    AudioProcessorParameter* getBypassParameter() const override;

    //==========================================================================
    void getStateInformation(MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    //==========================================================================
    AudioProcessorValueTreeState apvts;

    // Shame Extreme — the all-buttons-in mode entered by double-clicking the
    // Shame knob. Deliberately a recallable state property, NOT a host
    // parameter: the parameter set stays exactly as it always was.
    bool isShameExtreme() const noexcept                 { return shameExtreme.load(); }
    void setShameExtreme(bool extreme) noexcept          { shameExtreme.store(extreme); }

    // UI era preference ("heritage" | "modern"), persisted with the session.
    String getUIEra() const;
    void setUIEra(const String& era);

    bool getShowReels() const noexcept                   { return showReels.load(); }
    void setShowReels(bool show) noexcept                { showReels.store(show); }

    float getUIScale() const noexcept                    { return uiScale.load(); }
    void setUIScale(float scale) noexcept                { uiScale.store(jlimit(0.6f, 2.0f, scale)); }

    //==========================================================================
    float getCurrentRMSL() const noexcept                { return curRMSL.load(); }
    float getCurrentRMSR() const noexcept                { return curRMSR.load(); }
    bool isTransportPlaying() const noexcept             { return transportPlaying.load(); }
    int getBlockCounter() const noexcept                 { return blockCounter.load(); }
    bool isGraphBypassed() const noexcept                { return graphBypassed.load(); }

private:
    static AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    void updateGraphFromParameters();

    AudioGraph graph;

    std::atomic<float>* pInput        = nullptr;
    std::atomic<float>* pShame        = nullptr;
    std::atomic<float>* pHiss         = nullptr;
    std::atomic<float>* pAge          = nullptr;
    std::atomic<float>* pBlend        = nullptr;
    std::atomic<float>* pOutput       = nullptr;
    std::atomic<float>* pFlange       = nullptr;
    std::atomic<float>* pBypass       = nullptr;
    std::atomic<float>* pTapeType     = nullptr;
    std::atomic<float>* pPrintThrough = nullptr;
    std::atomic<float>* pEnvironment  = nullptr;

    std::atomic<bool> shameExtreme { false };
    std::atomic<bool> showReels { true };
    std::atomic<float> uiScale { 1.0f };

    CriticalSection eraLock;
    String uiEra { "heritage" };

    std::atomic<float> curRMSL { 0.0f };
    std::atomic<float> curRMSR { 0.0f };
    std::atomic<bool> transportPlaying { false };
    std::atomic<int> blockCounter { 0 };
    std::atomic<bool> graphBypassed { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(KissOfShameAudioProcessor)
};
