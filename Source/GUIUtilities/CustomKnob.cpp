#include "CustomKnob.h"

CustomKnob::CustomKnob()
{
    // Vertical OR horizontal drag both turn the knob (the original circular
    // gesture still works too) - and the web demo mirrors this exactly.
    setSliderStyle(RotaryHorizontalVerticalDrag);
    setTextBoxStyle(NoTextBox, true, 0, 0);
    setRange(0.000, 1.000, 0.001);
    setValue(0.0);
}

void CustomKnob::setNumFrames(int numFrames)
{
    knobNumFrames = jmax(1, numFrames);
}

void CustomKnob::setKnobImage(const Image& image)
{
    knobImage = image;
    repaint();
}

void CustomKnob::setKnobDimensions(int topLeftX, int topLeftY, int w, int h)
{
    setTopLeftPosition(topLeftX, topLeftY);
    knobFrameWidth = w;
    knobFrameHeight = h;
    setSize(knobFrameWidth, knobFrameHeight);
}

void CustomKnob::mouseEnter(const MouseEvent& event)
{
    hovering = true;
    repaint();
    Slider::mouseEnter(event);
}

void CustomKnob::mouseExit(const MouseEvent& event)
{
    hovering = false;
    repaint();
    Slider::mouseExit(event);
}

void CustomKnob::paint(Graphics& g)
{
    const double normalizedValue = valueToProportionOfLength(getValue());

    if (era == UIEra::modern)
    {
        // The SHAME knob: the gear-edged wheel carrying the glowing
        // inverted cross — the shelved design's centrepiece. Other knobs
        // are glossy balls ringed with LED dots.
        if (modernCross)
            ModernTheme::drawShameGear(g, getLocalBounds().toFloat(), (float) normalizedValue,
                                       extremeVisual, glowLevel, hovering);
        else
            ModernTheme::drawKnob(g, getLocalBounds().toFloat(), (float) normalizedValue,
                                  false, extremeVisual, hovering);
        return;
    }

    if (! knobImage.isNull())
    {
        const int frameNum = (int) (normalizedValue * (knobNumFrames - 1));
        juce::Rectangle<int> clipRect(0, frameNum * knobFrameHeight, knobFrameWidth, knobFrameHeight);
        g.drawImageAt(knobImage.getClippedImage(clipRect), 0, 0);
    }
}

void CustomKnob::mouseDown(const MouseEvent& event)
{
    // Match the web demo's drag feel exactly: 180 px of combined vertical and
    // horizontal travel sweeps the full range, and holding Shift quarters the
    // speed for fine adjustment.
    setMouseDragSensitivity(event.mods.isShiftDown() ? 720 : 180);
    Slider::mouseDown(event);
}

void CustomKnob::mouseDoubleClick(const MouseEvent& event)
{
    if (onDoubleClick != nullptr)
        onDoubleClick();
    else
        Slider::mouseDoubleClick(event);
}
