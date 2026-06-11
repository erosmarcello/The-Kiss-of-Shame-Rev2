//
//  EraSwitch.h
//  KissOfShame (Rev 2)
//
//  The era selector: flips the GUI between the original OS X-golden-era
//  skeuomorphism (2014) and the modern flat/dimensional macOS look (NOW).
//
//  Deliberately not a normal toggle. It's recessed into the faceplate like
//  the protected switches on real tape machines: press it IN and hold —
//  the plunger sinks, arms with a pink ring — then release to flip eras.
//  Tap it casually and it just springs back. You have to mean it.
//

#pragma once

#include "Theme.h"

class EraSwitch : public Component, private Timer
{
public:
    EraSwitch()
    {
        setMouseCursor(MouseCursor::PointingHandCursor);
    }

    ~EraSwitch() override = default;

    std::function<void()> onEraToggled;

    void setEra(UIEra newEra)
    {
        era = newEra;
        targetSlide = (era == UIEra::modern) ? 1.0f : 0.0f;
        startTimerHz(60);
    }

    //==========================================================================
    void mouseDown(const MouseEvent&) override
    {
        pressing = true;
        armed = false;
        pressStartMs = Time::getMillisecondCounter();
        startTimerHz(60);
    }

    void mouseUp(const MouseEvent&) override
    {
        pressing = false;

        if (armed)
        {
            armed = false;
            if (onEraToggled != nullptr)
                onEraToggled();
        }

        startTimerHz(60);
    }

    //==========================================================================
    void paint(Graphics& g) override
    {
        auto r = getLocalBounds().toFloat().reduced(1.0f);

        // The recessed well, engraved into either faceplate.
        const bool modern = (era == UIEra::modern);
        const Colour wellDark  = modern ? ModernTheme::panelDeep : Colour(0xff0c0a0a);
        const Colour wellLight = modern ? ModernTheme::panelRaised : Colour(0xff3b3233);

        g.setColour(wellDark);
        g.fillRoundedRectangle(r, r.getHeight() * 0.5f);

        // inner shadow: darker on top (light comes from above the panel)
        ColourGradient innerShadow(Colours::black.withAlpha(0.65f), r.getCentreX(), r.getY(),
                                   Colours::transparentBlack, r.getCentreX(), r.getY() + r.getHeight() * 0.7f, false);
        g.setGradientFill(innerShadow);
        g.fillRoundedRectangle(r, r.getHeight() * 0.5f);

        g.setColour(wellLight.withAlpha(0.8f));
        g.drawRoundedRectangle(r, r.getHeight() * 0.5f, 1.0f);

        // era labels engraved in the well
        g.setFont(ModernTheme::labelFont(8.5f));
        auto labels = r.reduced(8.0f, 2.0f);
        g.setColour((slide < 0.5f ? ModernTheme::accent : Colours::white.withAlpha(0.25f)));
        g.drawText("2014", labels.removeFromLeft(labels.getWidth() * 0.45f), Justification::centredLeft, false);
        g.setColour((slide >= 0.5f ? ModernTheme::accent : Colours::white.withAlpha(0.25f)));
        g.drawText("NOW", labels, Justification::centredRight, false);

        // the plunger: slides between eras, sinks while pressed
        const float thumbD = r.getHeight() - 6.0f;
        const float trackW = r.getWidth() - thumbD - 8.0f;
        const float thumbX = r.getX() + 4.0f + trackW * slide;
        const float sink = depress * 2.0f;

        juce::Rectangle<float> thumb(thumbX, r.getY() + 3.0f + sink * 0.5f, thumbD, thumbD);
        thumb = thumb.reduced(sink);

        if (armed)
        {
            g.setColour(ModernTheme::accent.withAlpha(0.55f));
            g.drawEllipse(thumb.expanded(3.0f), 2.0f);
        }

        if (modern)
        {
            g.setColour(depress > 0.5f ? ModernTheme::outline : ModernTheme::textPrimary);
            g.fillEllipse(thumb);
        }
        else
        {
            // brushed-chrome plunger for the heritage faceplate
            ColourGradient chrome(Colour(0xffd8d4d2), thumb.getCentreX(), thumb.getY(),
                                  Colour(0xff6f6a68), thumb.getCentreX(), thumb.getBottom(), false);
            g.setGradientFill(chrome);
            g.fillEllipse(thumb);
            g.setColour(Colour(0xff2b2627));
            g.drawEllipse(thumb, 1.0f);
        }

        // pressing dims the plunger — it's going INTO the panel
        if (depress > 0.01f)
        {
            g.setColour(Colours::black.withAlpha(0.35f * depress));
            g.fillEllipse(thumb);
        }
    }

private:
    void timerCallback() override
    {
        bool active = false;

        const float depressTarget = pressing ? 1.0f : 0.0f;
        if (std::abs(depress - depressTarget) > 0.01f)
        {
            depress += (depressTarget - depress) * 0.35f;
            active = true;
        }
        else
        {
            depress = depressTarget;
        }

        // Fully seated and held long enough: armed.
        if (pressing && ! armed && depress > 0.9f
            && Time::getMillisecondCounter() - pressStartMs >= holdToArmMs)
        {
            armed = true;
            active = true;
        }

        if (std::abs(slide - targetSlide) > 0.005f)
        {
            slide += (targetSlide - slide) * 0.42f; // snappy spring
            active = true;
        }
        else
        {
            slide = targetSlide;
        }

        repaint();

        if (! active && ! pressing)
            stopTimer();
    }

    UIEra era = UIEra::heritage;

    static constexpr uint32 holdToArmMs = 350;

    bool pressing = false;
    bool armed = false;
    uint32 pressStartMs = 0;

    float depress = 0.0f;
    float slide = 0.0f;
    float targetSlide = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EraSwitch)
};
