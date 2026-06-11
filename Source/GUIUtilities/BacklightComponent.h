#pragma once

#include "Theme.h"

// The glow behind the machine. Heritage: the original solid pink wash.
// Modern: a living ember — it breathes on its own slow cycle, swells with
// the program audio, and runs hot and fast in Extreme mode.
class BacklightComponent : public Component, private Timer
{
public:
    BacklightComponent()
    {
        setSize(960, 266);
        setInterceptsMouseClicks(false, false);
    }

    ~BacklightComponent() override = default;

    void setEra(UIEra newEra)
    {
        era = newEra;
        if (era == UIEra::modern)
            startTimerHz(30);
        else
            stopTimer();
        repaint();
    }

    void setExtreme(bool shouldBeExtreme)
    {
        extreme = shouldBeExtreme;
        repaint();
    }

    // Fed from the editor's meter poll; smoothed here.
    void setAudioLevel(float level01)
    {
        targetLevel = jlimit(0.0f, 1.0f, level01);
    }

    void paint(Graphics& g) override
    {
        const auto glow = extreme ? Colour::fromFloatRGBA(1.0f, 0.10f, 0.16f, 1.0f)
                                  : Colour::fromFloatRGBA(1.0f, 0.216f, 0.384f, 1.0f);

        if (era == UIEra::modern)
        {
            auto r = getLocalBounds().toFloat();

            const float breath = 0.5f + 0.5f * std::sin(phase);
            const float intensity = (extreme ? 0.13f : 0.045f)
                                  + (extreme ? 0.06f : 0.025f) * breath
                                  + 0.16f * level;

            // ember pooled beneath the cross (proportional: works for any
            // deck geometry the eras choose)
            const float ex = r.getWidth() * 0.5f, ey = r.getHeight() * 0.52f;
            ColourGradient ember(glow.withAlpha(intensity), ex, ey,
                                 glow.withAlpha(0.0f), ex, ey - r.getHeight() * 1.1f, true);
            g.setGradientFill(ember);
            g.fillRect(r);

            // rim light along the deck's lower edge
            ColourGradient rim(glow.withAlpha(intensity * 0.35f), r.getCentreX(), r.getBottom(),
                               glow.withAlpha(0.0f), r.getCentreX(), r.getBottom() - 46.0f, false);
            g.setGradientFill(rim);
            g.fillRect(r.withTop(r.getBottom() - 46.0f));
            return;
        }

        g.fillAll(glow);
    }

private:
    void timerCallback() override
    {
        phase += extreme ? 0.20f : 0.085f;
        if (phase > MathConstants<float>::twoPi)
            phase -= MathConstants<float>::twoPi;

        level += (targetLevel - level) * 0.22f;

        repaint();
    }

    UIEra era = UIEra::heritage;
    bool extreme = false;

    float phase = 0.0f;
    float level = 0.0f;
    float targetLevel = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BacklightComponent)
};
