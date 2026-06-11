#pragma once

#include "ImageInteractor.h"

// The storage-environment selector strip. Decoupled from the processor:
// clicks go out through onEnvironmentChanged, display state comes in via
// setDisplayedEnvironment — the parameter system stays the single source of
// truth (and host automation moves the strip).
class EnvironmentsComponent : public ImageInteractor
{
public:
    EnvironmentsComponent()
    {
        setNumFrames(6);
        setMinMaxValues(0, 5);
        setDimensions(0, 0, 183, 32);
        setMouseCursor(MouseCursor::PointingHandCursor);
    }

    ~EnvironmentsComponent() override = default;

    void setDisplayedEnvironment(int index)
    {
        currentIndex = jlimit(0, 5, index);
        updateImageWithValue((float) currentIndex);
    }

    void mouseUp(const MouseEvent& event) override
    {
        ignoreUnused(event);

        const int next = (currentIndex + 1) % 6;
        setDisplayedEnvironment(next);

        if (onEnvironmentChanged != nullptr)
            onEnvironmentChanged(next);
    }

    void paint(Graphics& g) override
    {
        if (getEra() == UIEra::modern)
        {
            static const char* names[] = { "OFF", "ENVIRONS", "STUDIO CLOSET",
                                           "HUMID CELLAR", "HOT LOCKER", "HURRICANE SANDY" };

            auto r = getLocalBounds().toFloat().reduced(1.0f);
            const bool active = currentIndex > 0;

            g.setColour(ModernTheme::panelRaised);
            g.fillRoundedRectangle(r, r.getHeight() * 0.5f);
            g.setColour(active ? ModernTheme::accent : ModernTheme::outline);
            g.drawRoundedRectangle(r, r.getHeight() * 0.5f, 1.2f);

            g.setColour(active ? ModernTheme::textPrimary : ModernTheme::textDim);
            g.setFont(ModernTheme::labelFont(11.0f));
            g.drawText(names[currentIndex], r, Justification::centred, false);
            return;
        }

        ImageInteractor::paint(g);
    }

    std::function<void(int)> onEnvironmentChanged;

private:
    int currentIndex = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EnvironmentsComponent)
};
