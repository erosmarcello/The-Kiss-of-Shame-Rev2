#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "GUIUtilities/CustomKnob.h"
#include "GUIUtilities/CustomButton.h"
#include "GUIUtilities/ImageInteractor.h"
#include "GUIUtilities/EnvironmentsComponent.h"
#include "GUIUtilities/BacklightComponent.h"
#include "GUIUtilities/ImageAnimationComponent.h"
#include "GUIUtilities/EraSwitch.h"

//==============================================================================
// The Kiss of Shame faceplate. Rev 2: parameters bind through APVTS
// attachments, images come from BinaryData, and double-clicking the Shame
// knob toggles Extreme mode (the tape equivalent of an LA-2A with all
// buttons in). Double-clicking the faceplate still collapses the reels.
class KissOfShameAudioProcessorEditor : public AudioProcessorEditor,
                                        private Timer
{
public:
    explicit KissOfShameAudioProcessorEditor(KissOfShameAudioProcessor&);
    ~KissOfShameAudioProcessorEditor() override = default;

    void paint(Graphics&) override;
    void paintOverChildren(Graphics&) override;
    void resized() override {}

    void mouseDoubleClick(const MouseEvent& event) override;

private:
    void timerCallback() override;
    void setReelMode(bool shouldShowReels);
    void applyReelVisibility();
    void toggleExtreme();
    void applyEra(UIEra newEra, bool animate);
    void paintModernPanel(Graphics& g);

    KissOfShameAudioProcessor& processor;

    //==========================================================================
    // Snapshot of the outgoing era, faded out over ~300ms when the switch
    // flips — changing decades should feel like an event.
    class EraTransitionOverlay : public Component, private Timer
    {
    public:
        explicit EraTransitionOverlay(const Image& outgoing) : snapshot(outgoing)
        {
            setInterceptsMouseClicks(false, false);
            startTimerHz(60);
        }

        std::function<void()> onFinished;

        void paint(Graphics& g) override
        {
            g.setOpacity(alpha);
            g.drawImage(snapshot, getLocalBounds().toFloat());
        }

    private:
        void timerCallback() override
        {
            alpha -= 0.055f;
            if (alpha <= 0.0f)
            {
                stopTimer();
                setVisible(false);
                if (onFinished != nullptr)
                    onFinished();
                return;
            }
            repaint();
        }

        Image snapshot;
        float alpha = 1.0f;
    };

    //==========================================================================
    BacklightComponent backlight;
    ImageInteractor faceImage;
    EnvironmentsComponent environmentsComponent;

    CustomKnob inputSaturationKnob, shameKnob, hissKnob, blendKnob, outputKnob, ageKnob;
    CustomButton bypassButton, tapeTypeButton, printThroughButton, linkIOButtonL, linkIOButtonR;

    std::unique_ptr<ImageAnimationComponent> reelAnimation;
    ImageInteractor vuMeterL, vuMeterR, shameKnobImage;

    EraSwitch eraSwitch;
    std::unique_ptr<EraTransitionOverlay> eraTransition;
    UIEra era = UIEra::heritage;

    Image faceWithReels, faceWithoutReels;

    //==========================================================================
    using SliderAttachment = AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = AudioProcessorValueTreeState::ButtonAttachment;

    std::unique_ptr<SliderAttachment> inputAttachment, shameAttachment, hissAttachment,
                                      blendAttachment, outputAttachment, ageAttachment;
    std::unique_ptr<ButtonAttachment> bypassAttachment, tapeTypeAttachment, printThroughAttachment;
    std::unique_ptr<ParameterAttachment> flangeAttachment, environmentAttachment;

    //==========================================================================
    bool showReels = true;
    bool linkIOMode = false;
    int priorBlockCount = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(KissOfShameAudioProcessorEditor)
};
