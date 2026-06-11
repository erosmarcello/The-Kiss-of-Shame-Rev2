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
    CustomButton() : Button({}) {}
    ~CustomButton() override = default;

    void setEra(UIEra newEra) { era = newEra; repaint(); }

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

    void paintButton(Graphics& g, bool /*highlighted*/, bool down) override
    {
        if (era == UIEra::modern)
        {
            const bool on = getToggleState();
            ModernTheme::drawButton(g, getLocalBounds().toFloat(), on ? labelOn : labelOff, on, down);
            return;
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
    String labelOff, labelOn;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CustomButton)
};
