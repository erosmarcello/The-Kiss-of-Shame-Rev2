#pragma once

#include "Theme.h"

// Value-driven filmstrip display (VU meters, shame ring, face background).
// In the modern era each instance declares what it becomes: hidden (the
// vector controls draw themselves) or a vector VU meter.
class ImageInteractor : public Component
{
public:
    enum class ModernStyle { hidden, vuMeter };

    ImageInteractor();
    ~ImageInteractor() override = default;

    void paint(Graphics& g) override;

    void setEra(UIEra newEra)              { era = newEra; repaint(); }
    void setModernStyle(ModernStyle style) { modernStyle = style; }
    UIEra getEra() const                   { return era; }

    void updateImageWithValue(float value)
    {
        // Needle ballistics: the displayed value eases toward the target so
        // meters swing like hardware instead of snapping like counters.
        const float target = jlimit(minValue, maxValue, value);
        curValue = curValue + (target - curValue) * 0.38f;
        repaint();
    }

    void setCurrentValue(float value)
    {
        curValue = jlimit(minValue, maxValue, value);
    }

    void setMinMaxValues(float min, float max)
    {
        minValue = min;
        maxValue = max;
    }

    void setDesaturate(bool shouldDesaturate)
    {
        desaturate = shouldDesaturate;
        repaint();
    }

    void setNumFrames(int numFrames);
    void setAnimationImage(const Image& newImage);
    void setDimensions(int topLeftX, int topLeftY, int w, int h);

protected:
    float getNormalizedValue() const
    {
        return (curValue - minValue) / (maxValue - minValue);
    }

private:
    bool desaturate = false;
    Image satImage;
    Image desatImage;
    int frameWidth = 0;
    int frameHeight = 0;
    int numFrames = 128;

    UIEra era = UIEra::heritage;
    ModernStyle modernStyle = ModernStyle::hidden;

    float maxValue = 1.0f, minValue = 0.0f, curValue = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ImageInteractor)
};
