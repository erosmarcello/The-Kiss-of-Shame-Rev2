#pragma once

#include "ImageInteractor.h"

// The storage-environment selector, revitalized for Rev 2.
//
// Rev 1 UX was opaque: a small strip you blind-cycled with clicks. Now:
//   - click still cycles through the environments (the fast, tactile feel)
//   - the chevron at the right edge (or a right-click) opens a list of all
//     five environments by name, each with a one-line sonic description, so
//     you can pick the one you want directly instead of hunting for it
//   - scroll-wheel steps prev/next
//   - hover affordance + tooltip so the strip reads as interactive
//   - a thin pink underline shows the AGE intensity actually applied to the
//     chosen environment, making the AGE and ENVIRONMENT coupling visible
//
// Still decoupled from the processor: clicks go out through
// onEnvironmentChanged, display state comes in via setDisplayedEnvironment,
// so the parameter stays the single source of truth and host automation
// moves the strip.
class EnvironmentsComponent : public ImageInteractor,
                              public SettableTooltipClient
{
public:
    struct EnvironmentInfo
    {
        const char* name;
        const char* blurb;
    };

    static constexpr int numEnvironments = 5;

    static const EnvironmentInfo& getInfo(int index)
    {
        static const EnvironmentInfo infos[numEnvironments] = {
            { "Off",             "Fresh reel. No storage damage." },
            { "Studio Closet",   "Dry darkness. Dulling, light dropouts, stray crackle." },
            { "Humid Cellar",    "Sticky-shed. Drowned highs, slow swell, damp rumble." },
            { "Hot Locker",      "Heat warp. Drifting pitch, sagging level, hard print-through." },
            { "Hurricane Sandy", "The flood reel. Bursts, grit, and survival." }
        };
        return infos[jlimit(0, numEnvironments - 1, index)];
    }

    EnvironmentsComponent()
    {
        // The heritage filmstrip (00.png) still has 6 frames from the era
        // when "Environs" existed; setDisplayedEnvironment maps around the
        // retired frame.
        setNumFrames(6);
        setMinMaxValues(0, 5);
        setDimensions(0, 0, 183, 32);
        setMouseCursor(MouseCursor::PointingHandCursor);
        setTooltip("Storage environment - click to cycle, click the arrow (or right-click) for the full list, scroll to step. AGE sets how long the reel suffered there.");
    }

    ~EnvironmentsComponent() override = default;

    //==========================================================================
    void setDisplayedEnvironment(int index)
    {
        currentIndex = jlimit(0, numEnvironments - 1, index);
        // 00.png frames: 0 = "ENVIRONMENT" header, 1 = "---OFF---",
        // 2..5 = the four storage environments.
        updateImageWithValue((float) (currentIndex + 1));
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
        if (! (event.mouseWasClicked() && getLocalBounds().contains(event.getPosition())))
            return;

        // The right-edge chevron (or a right-click) opens the full list, so
        // you can pick directly; everywhere else keeps the click-to-cycle feel.
        if (event.mods.isPopupMenu() || event.getPosition().x >= getWidth() - caretZoneW)
            showPicker();
        else
            select((currentIndex + 1) % numEnvironments);
    }

    void mouseWheelMove(const MouseEvent&, const MouseWheelDetails& wheel) override
    {
        if (wheel.deltaY == 0.0f)
            return;

        const int step = wheel.deltaY > 0.0f ? 1 : -1;
        select((currentIndex + step + numEnvironments) % numEnvironments);
    }

    //==========================================================================
    // The callout: all five environments, named and described, direct click.
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
            // The shelved-design strip: glowing pink "< ENVIRONMENT >" —
            // arrows step, the name shows when a storage is engaged.
            auto r = getLocalBounds().toFloat();
            const bool active = currentIndex > 0;
            const String label = active ? String(getInfo(currentIndex).name).toUpperCase()
                                        : String("ENVIRONMENT");
            const Colour glow = ModernTheme::accent.brighter(hovering ? 0.25f : 0.05f);

            g.setFont(ModernTheme::labelFont(13.0f));

            // bloom pass under the crisp pass
            g.setColour(glow.withAlpha(0.35f));
            for (auto off : { -1.0f, 1.0f })
                g.drawText(label, r.translated(off, 0.0f), Justification::centred, false);
            g.setColour(glow);
            g.drawText(label, r, Justification::centred, false);

            // step arrows
            g.setColour(glow.withAlpha(hovering ? 1.0f : 0.75f));
            Path larrow, rarrow;
            const float cy = r.getCentreY(), ah = 5.5f;
            larrow.addTriangle(r.getX() + 2.0f, cy, r.getX() + 12.0f, cy - ah, r.getX() + 12.0f, cy + ah);
            rarrow.addTriangle(r.getRight() - 2.0f, cy, r.getRight() - 12.0f, cy - ah, r.getRight() - 12.0f, cy + ah);
            g.fillPath(larrow);
            g.fillPath(rarrow);

            drawAgeUnderline(g, r.reduced(20.0f, 1.0f));
            return;
        }

        ImageInteractor::paint(g);

        auto r = getLocalBounds().toFloat();

        if (hovering)
        {
            g.setColour(ModernTheme::accent.withAlpha(0.35f));
            g.drawRoundedRectangle(r.reduced(0.5f), 4.0f, 1.4f);
        }

        // List affordance: a small pink chevron tucked against the right edge.
        // Clicking it (or right-clicking the window) opens the full picker;
        // it brightens on hover so the "there is a list here" reads at a glance.
        {
            const float cx = r.getRight() - 12.0f;
            const float cy = r.getCentreY();
            Path chevron;
            chevron.startNewSubPath(cx - 4.0f, cy - 2.0f);
            chevron.lineTo(cx, cy + 2.5f);
            chevron.lineTo(cx + 4.0f, cy - 2.0f);
            g.setColour(ModernTheme::accent.withAlpha(hovering ? 0.95f : 0.45f));
            g.strokePath(chevron, PathStrokeType(1.6f, PathStrokeType::curved, PathStrokeType::rounded));
        }

        drawAgeUnderline(g, r.reduced(10.0f, 0.0f).withTrimmedRight(16.0f));
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
    static constexpr int caretZoneW = 26; // right-edge hit zone that opens the list

    int currentIndex = 0;
    float ageAmount = 0.0f;
    bool hovering = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EnvironmentsComponent)
};
