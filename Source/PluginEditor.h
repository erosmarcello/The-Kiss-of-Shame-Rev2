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
#include "GUIUtilities/FrostedStatusBar.h"

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
    void resized() override;

    // Headless hook for the snapshot tool / GUI tests.
    void showEnvironmentPicker() { environmentsComponent.showPicker(); }

private:
    void timerCallback() override;
    void setReelMode(bool shouldShowReels);
    void applyReelVisibility();
    void toggleReels();
    void toggleExtreme();
    void applyEra(UIEra newEra, bool animate);
    void positionEraDependentControls();
    void updateSizeConstraints();
    int logicalHeight() const;
    void refreshCanvasSize();
    void paintModernPanel(Graphics& g);
    void paintContentOverChildren(Graphics& g);

    //==========================================================================
    // All controls live on this fixed 960-wide logical canvas; the editor
    // scales it uniformly, so the UI is resizable and razor-sharp at any
    // size (vectors redraw, they don't stretch).
    class ScaledContent : public Component
    {
    public:
        std::function<void(Graphics&)> onPaint, onPaintOver;
        std::function<void()> onDoubleClick;

        void paint(Graphics& g) override              { if (onPaint) onPaint(g); }
        void paintOverChildren(Graphics& g) override  { if (onPaintOver) onPaintOver(g); }
        void mouseDoubleClick(const MouseEvent&) override { if (onDoubleClick) onDoubleClick(); }
    };

    KissOfShameAudioProcessor& processor;

    ScaledContent content;

    //==========================================================================
    // The decade change: the outgoing era lifts off the panel — eased
    // crossfade with a slight zoom while the signature pink flares and
    // recedes, like a lamp marking the jump. ~220ms, 60fps, fully eased;
    // fast enough to feel instant, soft enough to feel expensive.
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
            const float eased = t < 0.5f ? 2.0f * t * t
                                         : 1.0f - std::pow(-2.0f * t + 2.0f, 2.0f) / 2.0f;

            // outgoing era: fade + gentle lift
            const float zoom = 1.0f + 0.020f * eased;
            const auto centre = getLocalBounds().toFloat().getCentre();
            g.setOpacity(1.0f - eased);
            g.drawImageTransformed(snapshot,
                                   AffineTransform::scale(zoom, zoom, centre.x, centre.y)
                                       .scaled((float) getWidth() / (float) jmax(1, snapshot.getWidth()),
                                               (float) getHeight() / (float) jmax(1, snapshot.getHeight()),
                                               0.0f, 0.0f));

            // the pink flare: blooms early, gone by the end
            const float flare = std::sin(MathConstants<float>::pi * jmin(1.0f, t * 1.5f));
            g.setOpacity(1.0f);
            g.setColour(ModernTheme::accent.withAlpha(0.14f * flare));
            g.fillAll();
        }

    private:
        void timerCallback() override
        {
            t += 1.0f / (60.0f * 0.22f); // ~220ms
            if (t >= 1.0f)
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
        float t = 0.0f;
    };

    //==========================================================================
    BacklightComponent backlight;
    ImageInteractor faceImage;
    EnvironmentsComponent environmentsComponent;

    CustomKnob inputSaturationKnob, shameKnob, hissKnob, blendKnob, outputKnob, ageKnob;
    CustomButton bypassButton, bypassLever, tapeTypeButton, printThroughButton, linkIOButtonL, linkIOButtonR;

    std::unique_ptr<ImageAnimationComponent> reelAnimation;
    ImageInteractor vuMeterL, vuMeterR, shameKnobImage;

    EraSwitch eraSwitch;
    FrostedStatusBar statusBar;
    std::unique_ptr<EraTransitionOverlay> eraTransition;
    UIEra era = UIEra::heritage;

    TooltipWindow tooltipWindow { this, 700 };

    Image faceWithReels, faceWithoutReels;

    //==========================================================================
    using SliderAttachment = AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = AudioProcessorValueTreeState::ButtonAttachment;

    std::unique_ptr<SliderAttachment> inputAttachment, shameAttachment, hissAttachment,
                                      blendAttachment, outputAttachment, ageAttachment;
    std::unique_ptr<ButtonAttachment> bypassAttachment, bypassLeverAttachment,
                                      tapeTypeAttachment, printThroughAttachment;
    std::unique_ptr<ParameterAttachment> flangeAttachment, environmentAttachment;

    //==========================================================================
    bool showReels = true;
    bool linkIOMode = false;
    bool constructionComplete = false;
    int priorBlockCount = 0;
    float pulseLevel = 0.0f; // secondary metering: program level driving the control glints

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(KissOfShameAudioProcessorEditor)
};
