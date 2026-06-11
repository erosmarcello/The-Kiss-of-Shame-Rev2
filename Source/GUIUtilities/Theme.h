//
//  Theme.h
//  KissOfShame (Rev 2)
//
//  The two faces of the machine:
//
//    HERITAGE — the original 2014 skeuomorphic identity: Yannick Bonnefoy's
//    filmstrip knobs, bitmap VU meters, the photographed faceplate. Untouched.
//
//    MODERN — the same instrument re-drawn in the flat, dimensional language
//    of recent macOS: near-black materials, continuous rounding, restrained
//    depth, and the signature pink as the single accent. Fully vector, so it
//    scales to any display.
//
//  Components never decide colors themselves; they branch on UIEra and call
//  these helpers, so each era stays coherent everywhere.
//

#pragma once

#include "../shameConfig.h"

enum class UIEra
{
    heritage,
    modern
};

namespace ModernTheme
{
    // Palette: macOS-dark materials + the signature pink of the cross logo.
    inline const Colour panel        { 0xff1c1c1e };
    inline const Colour panelRaised  { 0xff2c2c2e };
    inline const Colour panelDeep    { 0xff111113 };
    inline const Colour outline      { 0xff3a3a3c };
    inline const Colour textPrimary  { 0xffe8e8ea };
    inline const Colour textDim      { 0xff98989d };
    inline const Colour accent       { Colour::fromFloatRGBA(1.0f, 0.216f, 0.384f, 1.0f) };
    inline const Colour accentHot    { Colour::fromFloatRGBA(1.0f, 0.10f, 0.16f, 1.0f) };

    inline constexpr float rotaryStart = -2.356f; // -135 degrees from 12 o'clock
    inline constexpr float rotaryEnd   =  2.356f;

    inline Font labelFont(float height)
    {
        return Font(FontOptions().withHeight(height)).boldened();
    }

    //==========================================================================
    inline void drawKnob(Graphics& g, juce::Rectangle<float> bounds, float value01,
                         bool drawCross = false, bool extreme = false)
    {
        auto r = bounds.reduced(bounds.getWidth() * 0.10f);
        const auto centre = r.getCentre();
        const float radius = r.getWidth() * 0.5f;
        const float angle = rotaryStart + value01 * (rotaryEnd - rotaryStart);

        if (extreme)
        {
            ColourGradient glow(accentHot.withAlpha(0.40f), centre.x, centre.y,
                                accentHot.withAlpha(0.0f), centre.x, centre.y - radius * 1.5f, true);
            g.setGradientFill(glow);
            g.fillEllipse(bounds.expanded(6.0f));
        }

        // body
        ColourGradient body(panelRaised, centre.x, r.getY(), panelDeep, centre.x, r.getBottom(), false);
        g.setGradientFill(body);
        g.fillEllipse(r);
        g.setColour(outline);
        g.drawEllipse(r, 1.5f);

        // track + value arcs
        const float arcRadius = radius + bounds.getWidth() * 0.06f;
        Path track;
        track.addCentredArc(centre.x, centre.y, arcRadius, arcRadius, 0.0f, rotaryStart, rotaryEnd, true);
        g.setColour(outline);
        g.strokePath(track, PathStrokeType(2.5f, PathStrokeType::curved, PathStrokeType::rounded));

        if (value01 > 0.001f)
        {
            Path value;
            value.addCentredArc(centre.x, centre.y, arcRadius, arcRadius, 0.0f, rotaryStart, angle, true);
            g.setColour(extreme ? accentHot : accent);
            g.strokePath(value, PathStrokeType(2.5f, PathStrokeType::curved, PathStrokeType::rounded));
        }

        // pointer
        Path pointer;
        pointer.addRoundedRectangle(-1.5f, -radius * 0.92f, 3.0f, radius * 0.34f, 1.5f);
        g.setColour(textPrimary);
        g.fillPath(pointer, AffineTransform::rotation(angle).translated(centre.x, centre.y));

        // the cross — brand continuity on the Shame knob
        if (drawCross)
        {
            const float arm = radius * 0.42f;
            const float thick = jmax(3.0f, radius * 0.14f);
            g.setColour(extreme ? accentHot : accent);
            g.fillRoundedRectangle(centre.x - thick * 0.5f, centre.y - arm, thick, arm * 2.0f, thick * 0.4f);
            g.fillRoundedRectangle(centre.x - arm, centre.y - thick * 0.5f, arm * 2.0f, thick, thick * 0.4f);
        }
    }

    //==========================================================================
    inline void drawVUMeter(Graphics& g, juce::Rectangle<float> bounds, float value01)
    {
        auto r = bounds.reduced(2.0f);
        g.setColour(panelRaised);
        g.fillRoundedRectangle(r, 8.0f);
        g.setColour(outline);
        g.drawRoundedRectangle(r, 8.0f, 1.2f);

        const auto pivot = Point<float>(r.getCentreX(), r.getBottom() - r.getHeight() * 0.18f);
        const float needleLen = r.getHeight() * 0.62f;
        const float minAngle = -0.85f, maxAngle = 0.85f;

        // scale ticks
        g.setColour(textDim);
        for (int t = 0; t <= 6; ++t)
        {
            const float a = minAngle + (maxAngle - minAngle) * (float) t / 6.0f;
            const auto p1 = pivot.getPointOnCircumference(needleLen, a);
            const auto p2 = pivot.getPointOnCircumference(needleLen - 5.0f, a);
            g.drawLine({ p1, p2 }, t >= 5 ? 1.8f : 1.0f);
        }

        // red zone
        Path redZone;
        redZone.addCentredArc(pivot.x, pivot.y, needleLen + 3.0f, needleLen + 3.0f, 0.0f,
                              minAngle + (maxAngle - minAngle) * 0.78f, maxAngle, true);
        g.setColour(accent);
        g.strokePath(redZone, PathStrokeType(2.0f));

        // needle
        const float needleAngle = minAngle + (maxAngle - minAngle) * jlimit(0.0f, 1.0f, value01);
        g.setColour(textPrimary);
        g.drawLine(Line<float>(pivot, pivot.getPointOnCircumference(needleLen - 2.0f, needleAngle)), 2.0f);

        g.setColour(panelDeep);
        g.fillEllipse(pivot.x - 4.0f, pivot.y - 4.0f, 8.0f, 8.0f);

        g.setColour(textDim);
        g.setFont(labelFont(10.0f));
        g.drawText("VU", r.removeFromBottom(14.0f), Justification::centred, false);
    }

    //==========================================================================
    inline void drawReel(Graphics& g, Point<float> centre, float radius, float angleRad)
    {
        // platter
        ColourGradient body(panelRaised, centre.x, centre.y - radius, panelDeep, centre.x, centre.y + radius, false);
        g.setGradientFill(body);
        g.fillEllipse(centre.x - radius, centre.y - radius, radius * 2, radius * 2);
        g.setColour(outline);
        g.drawEllipse(centre.x - radius, centre.y - radius, radius * 2, radius * 2, 2.0f);

        // tape pack
        const float packR = radius * 0.82f;
        g.setColour(panelDeep.darker(0.3f));
        g.fillEllipse(centre.x - packR, centre.y - packR, packR * 2, packR * 2);

        // three spoke windows, rotating
        for (int s = 0; s < 3; ++s)
        {
            const float a = angleRad + (float) s * MathConstants<float>::twoPi / 3.0f;
            Path window;
            window.addCentredArc(centre.x, centre.y, radius * 0.52f, radius * 0.52f, a, -0.42f, 0.42f, true);
            window.addCentredArc(centre.x, centre.y, radius * 0.24f, radius * 0.24f, a, 0.42f, -0.42f, false);
            window.closeSubPath();
            g.setColour(panel.brighter(0.06f));
            g.fillPath(window);
        }

        // hub
        g.setColour(accent);
        g.fillEllipse(centre.x - radius * 0.12f, centre.y - radius * 0.12f, radius * 0.24f, radius * 0.24f);
        g.setColour(panelDeep);
        g.fillEllipse(centre.x - radius * 0.045f, centre.y - radius * 0.045f, radius * 0.09f, radius * 0.09f);
    }

    //==========================================================================
    inline void drawButton(Graphics& g, juce::Rectangle<float> bounds, const String& label,
                           bool on, bool pressed)
    {
        auto r = bounds.reduced(1.0f);

        if (on)
        {
            g.setColour(pressed ? accent.darker(0.3f) : accent);
            g.fillRoundedRectangle(r, 6.0f);
            g.setColour(panelDeep);
        }
        else
        {
            g.setColour(pressed ? panelDeep : panelRaised);
            g.fillRoundedRectangle(r, 6.0f);
            g.setColour(outline);
            g.drawRoundedRectangle(r, 6.0f, 1.2f);
            g.setColour(textDim);
        }

        g.setFont(labelFont(jmin(11.0f, r.getHeight() * 0.42f)));
        g.drawText(label, r, Justification::centred, false);
    }
}
