#pragma once

#include "Theme.h"

// Rotary knob with two faces: heritage filmstrip frames, or the vector
// modern rendering. A double-click hook is exposed — the Shame knob uses it
// to enter Extreme mode.
class CustomKnob : public Slider
{
public:
    CustomKnob();
    ~CustomKnob() override = default;

    void setNumFrames(int numFrames);
    void setKnobImage(const Image& image);
    void setKnobDimensions(int topLeftX, int topLeftY, int w, int h);

    void setEra(UIEra newEra)            { era = newEra; repaint(); }
    void setModernCross(bool cross)      { modernCross = cross; }
    void setExtremeVisual(bool extreme)  { extremeVisual = extreme; repaint(); }

    // Audio-reactive halo for the cross: the logo breathes with the program.
    void setGlowLevel(float level01)
    {
        level01 = jlimit(0.0f, 1.0f, level01);
        if (std::abs(level01 - glowLevel) > 0.015f)
        {
            glowLevel = level01;
            if (era == UIEra::modern && modernCross)
                repaint();
        }
    }

    void paint(Graphics& g) override;
    void mouseDoubleClick(const MouseEvent& event) override;
    void mouseEnter(const MouseEvent& event) override;
    void mouseExit(const MouseEvent& event) override;

    std::function<void()> onDoubleClick;

private:
    int knobNumFrames = 128;

    Image knobImage;
    int knobFrameWidth = 0;
    int knobFrameHeight = 0;

    UIEra era = UIEra::heritage;
    bool modernCross = false;
    bool extremeVisual = false;
    bool hovering = false;
    float glowLevel = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CustomKnob)
};
