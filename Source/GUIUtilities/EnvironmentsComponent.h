#pragma once

#include "ImageInteractor.h"

// The storage-environment selector, revitalized for Rev 2.
//
// Rev 1 UX was opaque: a small strip you blind-cycled with clicks. Now:
//   - click opens a picker listing all six environments by name, each with
//     a one-line sonic description — direct selection, no cycling
//   - scroll-wheel steps prev/next (the old cycling, made deliberate)
//   - hover affordance + tooltip so the strip reads as interactive
//   - a thin pink underline shows the AGE intensity actually applied to the
//     chosen environment, making the AGE <-> ENVIRONMENT coupling visible
//
// Still decoupled from the processor: clicks go out through
// onEnvironmentChanged, display state comes in via setDisplayedEnvironment —
// the parameter stays the single source of truth and host automation moves
// the strip.
class EnvironmentsComponent : public ImageInteractor,
                              public SettableTooltipClient
{
public:
    struct EnvironmentInfo
    {
        const char* name;
        const char* blurb;
    };

    static constexpr int numEnvironments = 6;

    static const EnvironmentInfo& getInfo(int index)
    {
        static const EnvironmentInfo infos[numEnvironments] = {
            { "Off",             "Fresh reel. No storage damage." },
            { "Environs",        "Decades on a shelf. Gentle wear." },
            { "Studio Closet",   "Dry darkness. Dulling, light dropouts, stray crackle." },
            { "Humid Cellar",    "Sticky-shed. Drowned highs, slow swell, damp rumble." },
            { "Hot Locker",      "Heat warp. Drifting pitch, sagging level, hard print-through." },
            { "Hurricane Sandy", "The flood reel. Bursts, grit, and survival." }
        };
        return infos[jlimit(0, numEnvironments - 1, index)];
    }

    EnvironmentsComponent()
    {
        setNumFrames(numEnvironments);
        setMinMaxValues(0, (float) (numEnvironments - 1));
        setDimensions(0, 0, 183, 32);
        setMouseCursor(MouseCursor::PointingHandCursor);
        setTooltip("Storage environment - click to choose, scroll to step. AGE sets how long the reel suffered there.");
    }

    ~EnvironmentsComponent() override = default;

    //==========================================================================
    void setDisplayedEnvironment(int index)
    {
        currentIndex = jlimit(0, numEnvironments - 1, index);
        updateImageWithValue((float) currentIndex);
    }

    int getCurrentIndex() const { return currentIndex; }

    // Fed by the AGE knob so the strip can show the intensity actually
    // hitting the selected environment.
    void setAgeAmount(float age01)
    {
        ageAmount = jlimit(0.0f, 1.0f, age01);
        repaint();
    }

    float getAgeAmount() const { return ageAmount; }

    std::function<void(int)> onEnvironmentChanged;

    //==========================================================================
    void mouseEnter(const MouseEvent&) override { hovering = true; repaint(); }
    void mouseExit(const MouseEvent&) override  { hovering = false; repaint(); }

    void mouseUp(const MouseEvent& event) override
    {
        if (event.mouseWasClicked() && getLocalBounds().contains(event.getPosition()))
            showPicker();
    }

    void mouseWheelMove(const MouseEvent&, const MouseWheelDetails& wheel) override
    {
        if (wheel.deltaY == 0.0f)
            return;

        const int step = wheel.deltaY > 0.0f ? 1 : -1;
        select((currentIndex + step + numEnvironments) % numEnvironments);
    }

    //==========================================================================
    // The callout: all six environments, named and described, direct click.
    // Public so the headless snapshot tool can render it for docs/review.
    class Picker : public Component
    {
    public:
        Picker(UIEra eraIn, int currentIn, float ageIn, std::function<void(int)> onPickIn)
            : era(eraIn), current(currentIn), age(ageIn), onPick(std::move(onPickIn))
        {
            setSize(322, headerH + rowH * numEnvironments + footerH + 2 * pad);
        }

        void paint(Graphics& g) override
        {
            const bool modern = (era == UIEra::modern);

            const Colour bg      = modern ? ModernTheme::panel : Colour(0xff171211);
            const Colour bgHover = modern ? ModernTheme::panelRaised : Colour(0xff2a201e);
            const Colour text    = modern ? ModernTheme::textPrimary : Colour(0xffe6ded2);
            const Colour dim     = modern ? ModernTheme::textDim : Colour(0xff97897c);

            g.fillAll(bg);

            auto r = getLocalBounds().reduced(pad);

            // header
            auto header = r.removeFromTop(headerH);
            g.setColour(dim);
            g.setFont(ModernTheme::labelFont(10.0f));
            g.drawText("STORAGE ENVIRONMENT", header, Justification::centred, false);

            // rows
            for (int i = 0; i < numEnvironments; ++i)
            {
                auto row = r.removeFromTop(rowH);
                const bool selected = (i == current);
                const bool hovered = (i == hoverRow);

                if (hovered || selected)
                {
                    g.setColour(hovered ? bgHover : bgHover.withAlpha(0.55f));
                    g.fillRoundedRectangle(row.toFloat().reduced(2.0f), 7.0f);
                }

                auto inner = row.reduced(12, 4);

                // selection lamp
                auto lamp = inner.removeFromLeft(14);
                g.setColour(selected ? ModernTheme::accent : dim.withAlpha(0.35f));
                g.fillEllipse(lamp.withSizeKeepingCentre(7, 7).toFloat());

                inner.removeFromLeft(8);
                const auto& info = getInfo(i);

                g.setColour(selected ? ModernTheme::accent : text);
                g.setFont(ModernTheme::labelFont(12.5f));
                g.drawText(info.name, inner.removeFromTop(17), Justification::centredLeft, false);

                g.setColour(dim);
                g.setFont(Font(FontOptions().withHeight(10.5f)));
                g.drawText(info.blurb, inner, Justification::centredLeft, true);
            }

            // footer: the AGE coupling, made visible
            auto footer = r.removeFromTop(footerH).reduced(12, 6);
            g.setColour(dim);
            g.setFont(ModernTheme::labelFont(9.5f));
            g.drawText(current > 0 ? "AGE = TIME SPENT HERE" : "AGE IDLES WHILE OFF",
                       footer.removeFromTop(13), Justification::centredLeft, false);

            auto bar = footer.removeFromTop(5).toFloat().withTrimmedTop(1.0f);
            g.setColour(Colours::white.withAlpha(0.10f));
            g.fillRoundedRectangle(bar, 2.0f);
            g.setColour(current > 0 ? ModernTheme::accent : dim.withAlpha(0.4f));
            g.fillRoundedRectangle(bar.withWidth(jmax(3.0f, bar.getWidth() * age)), 2.0f);
        }

        void mouseMove(const MouseEvent& event) override
        {
            const int row = rowAt(event.getPosition());
            if (row != hoverRow)
            {
                hoverRow = row;
                repaint();
            }
        }

        void mouseExit(const MouseEvent&) override
        {
            hoverRow = -1;
            repaint();
        }

        void mouseUp(const MouseEvent& event) override
        {
            const int row = rowAt(event.getPosition());
            if (row >= 0)
            {
                if (onPick != nullptr)
                    onPick(row);

                if (auto* box = findParentComponentOfClass<CallOutBox>())
                    box->dismiss();
            }
        }

    private:
        int rowAt(Point<int> p) const
        {
            const int top = pad + headerH;
            if (p.y < top || p.y >= top + rowH * numEnvironments)
                return -1;
            return (p.y - top) / rowH;
        }

        static constexpr int pad = 8, headerH = 24, rowH = 36, footerH = 30;

        UIEra era;
        int current;
        float age;
        int hoverRow = -1;
        std::function<void(int)> onPick;
    };

    //==========================================================================
    void showPicker()
    {
        auto* editor = findParentComponentOfClass<AudioProcessorEditor>();
        if (editor == nullptr)
            return;

        auto picker = std::make_unique<Picker>(getEra(), currentIndex, ageAmount,
                                               [this](int index) { select(index); });

        const auto area = editor->getLocalArea(this, getLocalBounds());
        CallOutBox::launchAsynchronously(std::move(picker), area, editor);
    }

    //==========================================================================
    void paint(Graphics& g) override
    {
        if (getEra() == UIEra::modern)
        {
            auto r = getLocalBounds().toFloat().reduced(1.0f);
            const bool active = currentIndex > 0;

            g.setColour(hovering ? ModernTheme::panelRaised.brighter(0.08f) : ModernTheme::panelRaised);
            g.fillRoundedRectangle(r, r.getHeight() * 0.5f);
            g.setColour(active ? ModernTheme::accent : (hovering ? ModernTheme::textDim : ModernTheme::outline));
            g.drawRoundedRectangle(r, r.getHeight() * 0.5f, 1.2f);

            g.setColour(active ? ModernTheme::textPrimary : ModernTheme::textDim);
            g.setFont(ModernTheme::labelFont(11.0f));
            auto text = r.reduced(14.0f, 0.0f);
            g.drawText(String(getInfo(currentIndex).name).toUpperCase(), text, Justification::centred, false);

            // chevron: this opens a menu
            g.setColour(ModernTheme::textDim);
            Path chevron;
            const float cx = r.getRight() - 13.0f, cy = r.getCentreY() - 1.5f;
            chevron.addTriangle(cx - 4.0f, cy, cx + 4.0f, cy, cx, cy + 4.5f);
            g.fillPath(chevron);

            drawAgeUnderline(g, r.reduced(16.0f, 0.0f));
            return;
        }

        ImageInteractor::paint(g);

        auto r = getLocalBounds().toFloat();

        if (hovering)
        {
            g.setColour(ModernTheme::accent.withAlpha(0.35f));
            g.drawRoundedRectangle(r.reduced(0.5f), 4.0f, 1.4f);
        }

        drawAgeUnderline(g, r.reduced(10.0f, 0.0f));
    }

private:
    void drawAgeUnderline(Graphics& g, juce::Rectangle<float> r)
    {
        if (currentIndex <= 0 || ageAmount <= 0.001f)
            return;

        auto track = r.removeFromBottom(2.5f).withTrimmedBottom(0.5f);
        g.setColour(Colours::white.withAlpha(0.10f));
        g.fillRoundedRectangle(track, 1.0f);
        g.setColour(ModernTheme::accent.withAlpha(0.85f));
        g.fillRoundedRectangle(track.withWidth(track.getWidth() * ageAmount), 1.0f);
    }

    void select(int index)
    {
        setDisplayedEnvironment(index);
        if (onEnvironmentChanged != nullptr)
            onEnvironmentChanged(currentIndex);
    }

    //==========================================================================
    int currentIndex = 0;
    float ageAmount = 0.0f;
    bool hovering = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EnvironmentsComponent)
};
