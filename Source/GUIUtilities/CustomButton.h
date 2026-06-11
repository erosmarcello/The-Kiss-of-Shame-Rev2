#pragma once

#include "Theme.h"

// Toggle button with two faces: heritage frames clipped from the faceplate
// filmstrips, or flat labelled pills in the modern era.
//
// Heritage image mapping preserves Rev 1's ImageButton behavior exactly:
// the "on" image shows while UNtoggled, the "off" image while toggled
// (that's how the original lamp/button frames were authored).
class CustomButton : public Button
{
public:
    // Modern-era render styles (heritage always uses the filmstrip images).
    enum class ModernStyle { pill, lampKnob, crossLamp };

    CustomButton() : Button({}) {}
    ~CustomButton() override = default;

    void setEra(UIEra newEra) { era = newEra; repaint(); }
    void setModernStyle(ModernStyle style) { modernStyle = style; }

    // Labels for the modern era: shown according to toggle state.
    void setModernLabels(const String& whenOff, const String& whenOn)
    {
        labelOff = whenOff;
        labelOn = whenOn;
    }

    void resizeButton(float scale)
    {
        imageUntoggled = imageUntoggled.rescaled((int) (imageUntoggled.getWidth() * scale),
                                                 (int) (imageUntoggled.getHeight() * scale));
        imageToggled = imageToggled.rescaled((int) (imageToggled.getWidth() * scale),
                                             (int) (imageToggled.getHeight() * scale));
        applySize();
    }

    void setClippedCustomOnImage(const Image& source, int topLeftX, int topLeftY, int w, int h)
    {
        if (! source.isNull())
            imageUntoggled = source.getClippedImage({ topLeftX, topLeftY, w, h });
        applySize();
    }

    void setClippedCustomOffImage(const Image& source, int topLeftX, int topLeftY, int w, int h)
    {
        if (! source.isNull())
            imageToggled = source.getClippedImage({ topLeftX, topLeftY, w, h });
        applySize();
    }

    void paintButton(Graphics& g, bool highlighted, bool down) override
    {
        if (era == UIEra::modern)
        {
            const bool on = getToggleState();
            auto r = getLocalBounds().toFloat();

            switch (modernStyle)
            {
            case ModernStyle::pill:
                ModernTheme::drawButton(g, r, on ? labelOn : labelOff, on, down, highlighted);
                return;

            case ModernStyle::lampKnob:
            {
                // BYPASS: a small ball with its pink lamp. Lamp lit = signal
                // flowing; dark = bypassed.
                auto knobArea = r.withTrimmedTop(r.getHeight() * 0.22f);
                ModernTheme::drawKnob(g, knobArea, on ? 0.0f : 1.0f, false, false, highlighted);

                const float lampR = jmax(3.0f, r.getWidth() * 0.085f);
                const auto lc = Point<float>(r.getX() + lampR + 2.0f, r.getY() + lampR + 1.0f);
                if (! on)
                {
                    g.setColour(ModernTheme::accent.withAlpha(0.4f));
                    g.fillEllipse(lc.x - lampR * 2.0f, lc.y - lampR * 2.0f, lampR * 4.0f, lampR * 4.0f);
                }
                g.setColour(on ? ModernTheme::accent.darker(1.6f) : ModernTheme::accent);
                g.fillEllipse(lc.x - lampR, lc.y - lampR, lampR * 2, lampR * 2);
                g.setColour(Colours::white.withAlpha(on ? 0.12f : 0.6f));
                g.fillEllipse(lc.x - lampR * 0.4f, lc.y - lampR * 0.5f, lampR * 0.8f, lampR * 0.8f);
                return;
            }

            case ModernStyle::crossLamp:
            {
                // Tape formulation: the small engraved cross with a lamp.
                // Lamp dark = S-111, lit = A-456.
                auto markArea = r.withTrimmedBottom(r.getHeight() * 0.30f).reduced(r.getWidth() * 0.22f, 0.0f);
                ModernTheme::drawInfernalLoveMark(g, markArea,
                                                  Colours::white.withAlpha(highlighted ? 0.5f : 0.32f),
                                                  ModernTheme::panel);

                const float lampR = jmax(2.6f, r.getWidth() * 0.10f);
                const auto lc = Point<float>(r.getCentreX(), r.getBottom() - lampR - 2.0f);
                if (on)
                {
                    g.setColour(ModernTheme::accent.withAlpha(0.45f));
                    g.fillEllipse(lc.x - lampR * 2.2f, lc.y - lampR * 2.2f, lampR * 4.4f, lampR * 4.4f);
                }
                g.setColour(on ? ModernTheme::accent : ModernTheme::accent.darker(1.8f));
                g.fillEllipse(lc.x - lampR, lc.y - lampR, lampR * 2, lampR * 2);
                g.setColour(Colours::white.withAlpha(on ? 0.6f : 0.10f));
                g.fillEllipse(lc.x - lampR * 0.4f, lc.y - lampR * 0.5f, lampR * 0.8f, lampR * 0.8f);
                return;
            }
            }
        }

        const Image& img = getToggleState() ? imageToggled : imageUntoggled;
        if (! img.isNull())
            g.drawImageAt(img, 0, 0);
    }

private:
    void applySize()
    {
        if (! imageUntoggled.isNull())
            setSize(imageUntoggled.getWidth(), imageUntoggled.getHeight());
    }

    Image imageUntoggled;
    Image imageToggled;

    UIEra era = UIEra::heritage;
    ModernStyle modernStyle = ModernStyle::pill;
    String labelOff, labelOn;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CustomButton)
};
