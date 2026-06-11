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
        // The Shame knob carries the Infernal Love logo — the inverted cross
        // with the heart. In the modern era we keep the ORIGINAL 2014
        // artwork, frame-accurate and still rotating, ringed by the vector
        // value arc. The brand is not negotiable.
        if (modernCross && ! knobImage.isNull())
        {
            const auto bounds = getLocalBounds().toFloat();
            const auto centre = bounds.getCentre();
            const float radius = bounds.getWidth() * 0.5f - bounds.getWidth() * 0.06f;

            if (extremeVisual)
            {
                ColourGradient glow(ModernTheme::accentHot.withAlpha(0.45f), centre.x, centre.y,
                                    ModernTheme::accentHot.withAlpha(0.0f), centre.x, bounds.getY() - 8.0f, true);
                g.setGradientFill(glow);
                g.fillEllipse(bounds.expanded(8.0f));
            }

            const int frameNum = (int) (normalizedValue * (knobNumFrames - 1));
            juce::Rectangle<int> clipRect(0, frameNum * knobFrameHeight, knobFrameWidth, knobFrameHeight);
            g.drawImage(knobImage.getClippedImage(clipRect), bounds, RectanglePlacement::centred);

            Path track;
            track.addCentredArc(centre.x, centre.y, radius, radius, 0.0f,
                                ModernTheme::rotaryStart, ModernTheme::rotaryEnd, true);
            g.setColour(ModernTheme::outline);
            g.strokePath(track, PathStrokeType(2.5f, PathStrokeType::curved, PathStrokeType::rounded));

            if (normalizedValue > 0.001)
            {
                const float angle = ModernTheme::rotaryStart
                                  + (float) normalizedValue * (ModernTheme::rotaryEnd - ModernTheme::rotaryStart);
                Path value;
                value.addCentredArc(centre.x, centre.y, radius, radius, 0.0f,
                                    ModernTheme::rotaryStart, angle, true);
                g.setColour(extremeVisual ? ModernTheme::accentHot : ModernTheme::accent);
                g.strokePath(value, PathStrokeType(2.5f, PathStrokeType::curved, PathStrokeType::rounded));
            }

            return;
        }

        ModernTheme::drawKnob(g, getLocalBounds().toFloat(), (float) normalizedValue,
                              false, extremeVisual);
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
