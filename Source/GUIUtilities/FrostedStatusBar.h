#pragma once

#include "EnvironmentsComponent.h"

// The frosted status spine. Fills the band between the reel bay and the
// control deck (modern era, reels shown) with the machine's live state:
// transport, tape formulation, environment, age, print-through and shame —
// readable at a glance from across the room, glowing on frosted glass.
//
// Mouse-transparent: it informs, the controls below act.
class FrostedStatusBar : public Component, private Timer
{
public:
    struct Providers
    {
        std::function<bool()> isA456, isPrintThrough, isExtreme, isPlaying, isBypassed;
        std::function<int()> environmentIndex;
        std::function<float()> age, shame, audioLevel;
    };

    FrostedStatusBar()
    {
        setInterceptsMouseClicks(false, false);
    }

    void setProviders(Providers p)
    {
        providers = std::move(p);
        startTimerHz(12);
    }

    void paint(Graphics& g) override
    {
        if (! providers.age)
            return;

        using namespace ModernTheme;

        auto r = getLocalBounds().toFloat();

        // frosted slab: translucent material, grain, lit top edge
        g.setColour(panelRaised.withAlpha(0.42f));
        g.fillRoundedRectangle(r, 12.0f);
        {
            Graphics::ScopedSaveState save(g);
            Path clip; clip.addRoundedRectangle(r, 12.0f);
            g.reduceClipRegion(clip);
            fillGrain(g, r, 0.28f);

            ColourGradient frost(Colours::white.withAlpha(0.05f), r.getCentreX(), r.getY(),
                                 Colours::transparentWhite, r.getCentreX(), r.getCentreY(), false);
            g.setGradientFill(frost);
            g.fillRect(r.withHeight(r.getHeight() * 0.5f));
        }
        g.setColour(Colours::white.withAlpha(0.14f));
        g.drawLine(r.getX() + 12.0f, r.getY() + 1.0f, r.getRight() - 12.0f, r.getY() + 1.0f, 1.0f);
        g.setColour(Colours::white.withAlpha(0.07f));
        g.drawRoundedRectangle(r, 12.0f, 1.0f);

        // six readout cells
        auto content = r.reduced(20.0f, 11.0f);
        const float cellW = content.getWidth() / 6.0f;

        const bool bypassed = providers.isBypassed();
        const bool playing = providers.isPlaying() && ! bypassed;
        const bool extreme = providers.isExtreme();
        const float breath = 0.5f + 0.5f * std::sin(phase);

        auto cell = [&](int index, const String& label) -> juce::Rectangle<float>
        {
            auto c = content.withX(content.getX() + cellW * (float) index).withWidth(cellW);

            if (index > 0)
            {
                g.setColour(Colours::black.withAlpha(0.35f));
                g.drawVerticalLine((int) c.getX(), c.getY() + 4.0f, c.getBottom() - 4.0f);
                g.setColour(Colours::white.withAlpha(0.05f));
                g.drawVerticalLine((int) c.getX() + 1, c.getY() + 4.0f, c.getBottom() - 4.0f);
            }

            g.setColour(textDim.withAlpha(0.75f));
            g.setFont(labelFont(8.5f));
            g.drawText(label, c.removeFromTop(14.0f).reduced(12.0f, 0.0f),
                       Justification::centredLeft, false);
            return c.reduced(12.0f, 2.0f);
        };

        // TRANSPORT: breathing lamp + state
        {
            auto c = cell(0, "TRANSPORT");
            auto lamp = c.removeFromLeft(16.0f).withSizeKeepingCentre(9.0f, 9.0f);
            const Colour lampColour = bypassed ? textDim.withAlpha(0.4f)
                                    : playing ? accent : textDim.withAlpha(0.6f);
            if (playing)
            {
                g.setColour(accent.withAlpha(0.25f + 0.25f * breath));
                g.fillEllipse(lamp.expanded(4.0f));
            }
            g.setColour(lampColour);
            g.fillEllipse(lamp);
            g.setColour(Colours::white.withAlpha(playing ? 0.5f : 0.15f));
            g.fillEllipse(lamp.reduced(2.8f).translated(-0.8f, -0.8f));

            g.setColour(playing ? textPrimary : textDim);
            g.setFont(labelFont(12.0f));
            g.drawText(bypassed ? "BYPASS" : (playing ? "ROLLING" : "STANDBY"),
                       c.withTrimmedLeft(4.0f), Justification::centredLeft, false);
        }

        // TAPE
        {
            auto c = cell(1, "TAPE");
            g.setColour(textPrimary);
            g.setFont(labelFont(12.0f));
            g.drawText(providers.isA456() ? "A-456" : "S-111", c, Justification::centredLeft, false);
        }

        // ENVIRONMENT
        {
            auto c = cell(2, "ENVIRONMENT");
            const int env = providers.environmentIndex();
            g.setColour(env > 0 ? accent : textDim);
            g.setFont(labelFont(11.5f));
            g.drawText(String(EnvironmentsComponent::getInfo(env).name).toUpperCase(),
                       c, Justification::centredLeft, false);
        }

        // AGE: value + meter
        {
            auto c = cell(3, "AGE");
            const float age = jlimit(0.0f, 1.0f, providers.age());
            g.setColour(textPrimary);
            g.setFont(labelFont(12.0f));
            g.drawText(String((int) std::lround(age * 100.0f)) + "%",
                       c.removeFromTop(15.0f), Justification::centredLeft, false);

            auto bar = c.removeFromTop(4.0f).withTrimmedRight(6.0f);
            g.setColour(Colours::white.withAlpha(0.10f));
            g.fillRoundedRectangle(bar, 2.0f);
            g.setColour(accent.withAlpha(0.9f));
            g.fillRoundedRectangle(bar.withWidth(jmax(3.0f, bar.getWidth() * age)), 2.0f);
        }

        // PRINT-THRU
        {
            auto c = cell(4, "PRINT-THRU");
            const bool on = providers.isPrintThrough();
            g.setColour(on ? accent : textDim);
            g.setFont(labelFont(12.0f));
            g.drawText(on ? "ECHOING" : "OFF", c, Justification::centredLeft, false);
        }

        // SHAME: value + EXTREME badge
        {
            auto c = cell(5, "SHAME");
            const float shame = jlimit(0.0f, 1.0f, providers.shame());
            g.setColour(extreme ? accentHot : textPrimary);
            g.setFont(labelFont(12.0f));
            g.drawText(String((int) std::lround(shame * 100.0f)) + "%",
                       c.removeFromLeft(42.0f), Justification::centredLeft, false);

            if (extreme)
            {
                auto badge = c.withSizeKeepingCentre(jmin(c.getWidth(), 64.0f), 15.0f);
                g.setColour(accentHot.withAlpha(0.30f + 0.35f * breath));
                g.fillRoundedRectangle(badge.expanded(2.0f), 5.0f);
                g.setColour(accentHot);
                g.fillRoundedRectangle(badge, 4.0f);
                g.setColour(Colours::white);
                g.setFont(labelFont(8.5f));
                g.drawText("EXTREME", badge, Justification::centred, false);
            }
        }
    }

private:
    void timerCallback() override
    {
        phase += providers.isExtreme && providers.isExtreme() ? 0.30f : 0.13f;
        if (phase > MathConstants<float>::twoPi)
            phase -= MathConstants<float>::twoPi;
        repaint();
    }

    Providers providers;
    float phase = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FrostedStatusBar)
};
