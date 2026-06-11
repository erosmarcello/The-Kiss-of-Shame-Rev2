#pragma once

#include "Theme.h"

// The glow behind the reels. Stock: the signature pink. Extreme mode runs
// hotter — the backlight shifts toward red so you always know the machine
// is past its stops. The modern era keeps only a low ember of it.
class BacklightComponent : public Component
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
        repaint();
    }

    void setExtreme(bool shouldBeExtreme)
    {
        extreme = shouldBeExtreme;
        repaint();
    }

    void paint(Graphics& g) override
    {
        const auto glow = extreme ? Colour::fromFloatRGBA(1.0f, 0.10f, 0.16f, 1.0f)
                                  : Colour::fromFloatRGBA(1.0f, 0.216f, 0.384f, 1.0f);

        if (era == UIEra::modern)
        {
            auto r = getLocalBounds().toFloat();
            ColourGradient ember(glow.withAlpha(extreme ? 0.28f : 0.14f), r.getCentreX(), r.getBottom(),
                                 Colours::transparentBlack, r.getCentreX(), r.getY(), false);
            g.setGradientFill(ember);
            g.fillRect(r);
            return;
        }

        g.fillAll(glow);
    }

private:
    UIEra era = UIEra::heritage;
    bool extreme = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BacklightComponent)
};
