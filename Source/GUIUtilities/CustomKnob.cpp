#include "CustomKnob.h"

CustomKnob::CustomKnob()
{
    setSliderStyle(Rotary);
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

void CustomKnob::paint(Graphics& g)
{
    const double normalizedValue = valueToProportionOfLength(getValue());

    if (era == UIEra::modern)
    {
        ModernTheme::drawKnob(g, getLocalBounds().toFloat(), (float) normalizedValue,
                              modernCross, extremeVisual);
        return;
    }

    if (! knobImage.isNull())
    {
        const int frameNum = (int) (normalizedValue * (knobNumFrames - 1));
        juce::Rectangle<int> clipRect(0, frameNum * knobFrameHeight, knobFrameWidth, knobFrameHeight);
        g.drawImageAt(knobImage.getClippedImage(clipRect), 0, 0);
    }
}

void CustomKnob::mouseDoubleClick(const MouseEvent& event)
{
    if (onDoubleClick != nullptr)
        onDoubleClick();
    else
        Slider::mouseDoubleClick(event);
}
