//
//  Theme.h
//  KissOfShame (Rev 2)
//
//  The two faces of the machine:
//
//    HERITAGE — the original 2014 skeuomorphic identity: Yannick Bonnefoy's
//    filmstrip knobs, bitmap VU meters, the photographed faceplate. Untouched.
//
//    MODERN — the shelved 2015 redesign, finally built: scratched gunmetal
//    faceplate, glossy ball knobs ringed with pink LED dots, the gear-edged
//    Shame knob carrying the glowing inverted cross, dark VU glass with the
//    cross watermark, silver large-format reels on wine-dark NAB hubs.
//    Fully vector, drawn with light — renders pixel-perfect at any scale.
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
    // Palette: weathered metal + the signature pink, glowing like neon.
    inline const Colour panel        { 0xff232326 };
    inline const Colour panelRaised  { 0xff2c2c2e };
    inline const Colour panelDeep    { 0xff101012 };
    inline const Colour outline      { 0xff3a3a3c };
    inline const Colour textPrimary  { 0xffe9e9ec };
    inline const Colour textDim      { 0xff9a9aa0 };
    inline const Colour accent       { Colour::fromFloatRGBA(1.0f, 0.216f, 0.384f, 1.0f) };
    inline const Colour accentHot    { Colour::fromFloatRGBA(1.0f, 0.10f, 0.16f, 1.0f) };
    inline const Colour silverHi     { 0xffd2d2d6 };
    inline const Colour silverLo     { 0xff85858c };
    inline const Colour wineHub      { 0xff4e1d2c };

    inline constexpr float rotaryStart = -2.356f; // -135 degrees from 12 o'clock
    inline constexpr float rotaryEnd   =  2.356f;

    inline Font labelFont(float height)
    {
        return Font(FontOptions().withHeight(height)).boldened().withExtraKerningFactor(0.08f);
    }

    //==========================================================================
    // Fine surface grain (subtle, tiled).
    inline const Image& grainTile()
    {
        static const Image tile = []
        {
            Image img(Image::ARGB, 128, 128, true);
            Random r(0x51043);
            for (int y = 0; y < 128; ++y)
                for (int x = 0; x < 128; ++x)
                {
                    const float v = r.nextFloat();
                    if (v > 0.5f)
                        img.setPixelAt(x, y, Colours::white.withAlpha((v - 0.5f) * 0.10f));
                    else
                        img.setPixelAt(x, y, Colours::black.withAlpha((0.5f - v) * 0.12f));
                }
            return img;
        }();
        return tile;
    }

    inline void fillGrain(Graphics& g, juce::Rectangle<float> r, float opacity = 1.0f)
    {
        g.setTiledImageFill(grainTile(), 0, 0, opacity);
        g.fillRect(r);
    }

    //==========================================================================
    // Heavy weathering: scratches, scuffs, stains and rust kisses — the
    // faceplate has lived. Cached 512px tile, deterministic.
    inline const Image& grungeTile()
    {
        static const Image tile = []
        {
            Image img(Image::ARGB, 512, 512, true);
            Graphics g(img);
            Random r(0xDEAD51);

            // stains and scuffs
            for (int i = 0; i < 46; ++i)
            {
                const float x = r.nextFloat() * 512.0f, y = r.nextFloat() * 512.0f;
                const float w = 18.0f + r.nextFloat() * 110.0f;
                const float h = w * (0.4f + r.nextFloat() * 0.8f);
                const bool rust = r.nextFloat() < 0.22f;
                const Colour c = rust ? Colour(0xff5a3a22).withAlpha(0.04f + r.nextFloat() * 0.05f)
                                      : (r.nextBool() ? Colours::black.withAlpha(0.05f + r.nextFloat() * 0.06f)
                                                      : Colours::white.withAlpha(0.02f + r.nextFloat() * 0.03f));
                ColourGradient blot(c, x, y, c.withAlpha(0.0f), x + w * 0.5f, y + h * 0.5f, true);
                g.setGradientFill(blot);
                g.fillEllipse(x - w * 0.5f, y - h * 0.5f, w, h);
            }

            // scratches: short hard nicks and long hairlines
            for (int i = 0; i < 170; ++i)
            {
                const float x = r.nextFloat() * 512.0f, y = r.nextFloat() * 512.0f;
                const float a = r.nextFloat() * MathConstants<float>::twoPi;
                const float len = r.nextFloat() < 0.18f ? 40.0f + r.nextFloat() * 150.0f
                                                        : 4.0f + r.nextFloat() * 26.0f;
                const float alpha = 0.03f + r.nextFloat() * 0.08f;
                g.setColour(r.nextBool() ? Colours::white.withAlpha(alpha)
                                         : Colours::black.withAlpha(alpha * 1.3f));
                g.drawLine(x, y, x + len * std::cos(a), y + len * std::sin(a),
                           r.nextFloat() < 0.85f ? 0.6f : 1.2f);
            }

            return img;
        }();
        return tile;
    }

    inline void fillGrunge(Graphics& g, juce::Rectangle<float> r, float opacity = 1.0f)
    {
        g.setTiledImageFill(grungeTile(), 0, 0, opacity);
        g.fillRect(r);
    }

    //==========================================================================
    inline void drawScrew(Graphics& g, Point<float> c, float radius, float slotAngle)
    {
        g.setColour(Colours::black.withAlpha(0.45f));
        g.fillEllipse(c.x - radius, c.y - radius + 1.2f, radius * 2, radius * 2);

        ColourGradient head(Colour(0xff47474b), c.x, c.y - radius,
                            Colour(0xff202022), c.x, c.y + radius, false);
        g.setGradientFill(head);
        g.fillEllipse(c.x - radius, c.y - radius, radius * 2, radius * 2);

        g.setColour(Colours::white.withAlpha(0.10f));
        g.drawEllipse(c.x - radius, c.y - radius, radius * 2, radius * 2, 0.8f);

        const auto p1 = c.getPointOnCircumference(radius * 0.72f, slotAngle);
        const auto p2 = c.getPointOnCircumference(radius * 0.72f, slotAngle + MathConstants<float>::pi);
        g.setColour(Colours::black.withAlpha(0.7f));
        g.drawLine({ p1, p2 }, 1.4f);
    }

    inline void dropShadowEllipse(Graphics& g, juce::Rectangle<float> r, float strength = 0.5f)
    {
        for (int i = 3; i >= 1; --i)
        {
            g.setColour(Colours::black.withAlpha(strength * 0.16f * (float) i));
            g.fillEllipse(r.translated(0.0f, 2.0f + (float) i).expanded((float) (4 - i)));
        }
    }

    //==========================================================================
    // Recessed seats: controls sit IN the deck, not on it.
    inline void drawRoundWell(Graphics& g, juce::Rectangle<float> r)
    {
        g.setColour(Colours::black.withAlpha(0.38f));
        g.fillEllipse(r);
        g.setColour(Colours::black.withAlpha(0.55f));
        g.drawEllipse(r.translated(0.0f, -0.8f), 1.6f);
        g.setColour(Colours::white.withAlpha(0.07f));
        g.drawEllipse(r.translated(0.0f, 0.9f), 1.0f);
    }

    inline void drawRectWell(Graphics& g, juce::Rectangle<float> r, float corner)
    {
        g.setColour(Colours::black.withAlpha(0.38f));
        g.fillRoundedRectangle(r, corner);
        g.setColour(Colours::black.withAlpha(0.55f));
        g.drawRoundedRectangle(r.translated(0.0f, -0.8f), corner, 1.6f);
        g.setColour(Colours::white.withAlpha(0.07f));
        g.drawRoundedRectangle(r.translated(0.0f, 0.9f), corner, 1.0f);
    }

    //==========================================================================
    // The dotted LED arc — the design's signature. Dots light progressively
    // with the value and bloom like small lamps.
    inline void drawDottedArc(Graphics& g, Point<float> centre, float radius, float value01,
                              int count, float dotRadius, Colour lit, bool allLitWhenIdle = false)
    {
        for (int i = 0; i < count; ++i)
        {
            const float frac = (float) i / (float) (count - 1);
            const float a = rotaryStart + frac * (rotaryEnd - rotaryStart);
            const auto p = centre.getPointOnCircumference(radius, a);
            const bool on = allLitWhenIdle || frac <= value01 + 0.0001f;

            if (on)
            {
                g.setColour(lit.withAlpha(0.30f));
                g.fillEllipse(p.x - dotRadius * 2.1f, p.y - dotRadius * 2.1f, dotRadius * 4.2f, dotRadius * 4.2f);
                g.setColour(lit);
                g.fillEllipse(p.x - dotRadius, p.y - dotRadius, dotRadius * 2, dotRadius * 2);
                g.setColour(Colours::white.withAlpha(0.65f));
                g.fillEllipse(p.x - dotRadius * 0.35f, p.y - dotRadius * 0.45f, dotRadius * 0.7f, dotRadius * 0.7f);
            }
            else
            {
                g.setColour(lit.darker(2.2f).withAlpha(0.55f));
                g.fillEllipse(p.x - dotRadius * 0.8f, p.y - dotRadius * 0.8f, dotRadius * 1.6f, dotRadius * 1.6f);
            }
        }
    }

    //==========================================================================
    // Glossy ball knob (INPUT/OUTPUT and the small AGE/HISS/BLEND family):
    // a polished black sphere in a recess, ringed by LED dots.
    inline void drawKnob(Graphics& g, juce::Rectangle<float> bounds, float value01,
                         bool /*drawCross*/ = false, bool extreme = false, bool hover = false)
    {
        const auto centre = bounds.getCentre();
        const float outer = bounds.getWidth() * 0.5f;
        const float ballR = outer * 0.70f;
        const float angle = rotaryStart + value01 * (rotaryEnd - rotaryStart);
        const Colour led = extreme ? accentHot : accent;
        auto ball = juce::Rectangle<float>(centre.x - ballR, centre.y - ballR, ballR * 2, ballR * 2);

        drawRoundWell(g, ball.expanded(ballR * 0.16f));
        dropShadowEllipse(g, ball, hover ? 0.7f : 0.55f);

        // the sphere: offset radial light, deep black floor
        {
            ColourGradient sphere(Colour(hover ? 0xff56565c : 0xff4c4c52),
                                  centre.x - ballR * 0.45f, centre.y - ballR * 0.55f,
                                  Colour(0xff060607), centre.x + ballR * 0.4f, centre.y + ballR * 0.9f, true);
            g.setGradientFill(sphere);
            g.fillEllipse(ball);

            // hard specular bite (top-left), soft bounce (bottom-right)
            ColourGradient spec(Colours::white.withAlpha(hover ? 0.50f : 0.40f),
                                centre.x - ballR * 0.40f, centre.y - ballR * 0.48f,
                                Colours::transparentWhite,
                                centre.x - ballR * 0.05f, centre.y - ballR * 0.02f, true);
            g.setGradientFill(spec);
            g.fillEllipse(ball);

            g.setColour(Colours::white.withAlpha(0.05f));
            g.drawEllipse(ball.reduced(ballR * 0.06f), 1.0f);
            g.setColour(Colours::black.withAlpha(0.75f));
            g.drawEllipse(ball, 1.2f);
        }

        // value indicator: a lit pin at the sphere's edge
        {
            const auto pin = centre.getPointOnCircumference(ballR * 0.78f, angle);
            g.setColour(Colours::black.withAlpha(0.6f));
            g.fillEllipse(pin.x - 3.2f, pin.y - 2.2f, 6.4f, 6.4f);
            g.setColour(Colours::white);
            g.fillEllipse(pin.x - 2.2f, pin.y - 2.2f, 4.4f, 4.4f);
            g.setColour(led.withAlpha(0.5f));
            g.drawEllipse(pin.x - 3.4f, pin.y - 3.4f, 6.8f, 6.8f, 1.2f);
        }

        // LED dots
        const int dots = bounds.getWidth() > 100.0f ? 21 : 13;
        drawDottedArc(g, centre, outer * 0.90f, value01, dots,
                      jmax(1.7f, outer * 0.040f), led);
    }

    //==========================================================================
    // The Infernal Love mark: inverted cross, heart at the crossing.
    inline void drawInfernalLoveMark(Graphics& g, juce::Rectangle<float> r,
                                     Colour crossColour, Colour heartCutout)
    {
        const float w = r.getWidth(), h = r.getHeight();
        const float cx = r.getCentreX();
        const float t = w * 0.30f;
        const float crossbarY = r.getY() + h * 0.62f;

        Path cross;
        cross.addRoundedRectangle(cx - t * 0.5f, r.getY(), t, h, t * 0.35f);
        cross.addRoundedRectangle(r.getX(), crossbarY - t * 0.5f, w, t, t * 0.35f);

        g.setColour(crossColour.withAlpha(0.30f));
        g.strokePath(cross, PathStrokeType(2.6f));
        g.setColour(crossColour);
        g.fillPath(cross);

        const float s = w * 0.52f;
        const float hy = crossbarY;
        Path heart;
        heart.startNewSubPath(cx, hy + 0.38f * s);
        heart.cubicTo(cx - 0.58f * s, hy + 0.02f * s, cx - 0.46f * s, hy - 0.40f * s, cx, hy - 0.12f * s);
        heart.cubicTo(cx + 0.46f * s, hy - 0.40f * s, cx + 0.58f * s, hy + 0.02f * s, cx, hy + 0.38f * s);
        heart.closeSubPath();
        g.setColour(heartCutout);
        g.fillPath(heart);
    }

    //==========================================================================
    // The SHAME knob: a wide ring of LED dots around a knurled gear-edge
    // wheel, the glowing cross with the heart at its centre, rotating with
    // the value. The altar of the interface.
    inline void drawShameGear(Graphics& g, juce::Rectangle<float> bounds, float value01,
                              bool extreme, float glowLevel, bool hover)
    {
        const auto centre = bounds.getCentre();
        const float outer = bounds.getWidth() * 0.5f;
        const float gearR = outer * 0.74f;
        const float faceR = gearR * 0.80f;
        const Colour led = extreme ? accentHot : accent;
        const float angle = rotaryStart + value01 * (rotaryEnd - rotaryStart);

        // ambient halo: breathes with the program, hotter in EXTREME
        {
            const float a = (extreme ? 0.34f : (hover ? 0.22f : 0.14f)) + 0.22f * glowLevel;
            ColourGradient halo(led.withAlpha(a), centre.x, centre.y,
                                led.withAlpha(0.0f), centre.x, centre.y - outer * 1.25f, true);
            g.setGradientFill(halo);
            g.fillEllipse(bounds.expanded(10.0f));
        }

        dropShadowEllipse(g, { centre.x - gearR, centre.y - gearR, gearR * 2, gearR * 2 }, 0.8f);

        // knurled gear edge: scalloped ring
        {
            Path gear;
            const int teeth = 28;
            for (int t = 0; t < teeth; ++t)
            {
                const float a0 = (float) t / teeth * MathConstants<float>::twoPi + angle * 0.4f;
                const float a1 = (float) (t + 1) / teeth * MathConstants<float>::twoPi + angle * 0.4f;
                const float mid = (a0 + a1) * 0.5f;
                const auto p0 = centre.getPointOnCircumference(gearR, a0);
                if (t == 0) gear.startNewSubPath(p0);
                const auto pn = centre.getPointOnCircumference(gearR * 0.955f, mid);
                const auto p1 = centre.getPointOnCircumference(gearR, a1);
                gear.quadraticTo(pn, p1);
            }
            gear.closeSubPath();

            ColourGradient metal(Colour(0xff3a3a3f), centre.x, centre.y - gearR,
                                 Colour(0xff0d0d0f), centre.x, centre.y + gearR, false);
            g.setGradientFill(metal);
            g.fillPath(gear);
            g.setColour(Colours::white.withAlpha(0.10f));
            g.strokePath(gear, PathStrokeType(1.0f));
        }

        // inner face: scratched black, slightly domed
        {
            ColourGradient face(Colour(0xff1f1f22), centre.x - faceR * 0.4f, centre.y - faceR * 0.5f,
                                Colour(0xff0a0a0c), centre.x + faceR * 0.3f, centre.y + faceR * 0.8f, true);
            g.setGradientFill(face);
            g.fillEllipse(centre.x - faceR, centre.y - faceR, faceR * 2, faceR * 2);

            Graphics::ScopedSaveState save(g);
            Path clip; clip.addEllipse(centre.x - faceR, centre.y - faceR, faceR * 2, faceR * 2);
            g.reduceClipRegion(clip);
            fillGrunge(g, { centre.x - faceR, centre.y - faceR, faceR * 2, faceR * 2 }, 0.8f);

            ColourGradient sheen(Colours::white.withAlpha(0.08f), centre.x - faceR * 0.5f, centre.y - faceR,
                                 Colours::transparentWhite, centre.x, centre.y, false);
            g.setGradientFill(sheen);
            g.fillRect(centre.x - faceR, centre.y - faceR, faceR * 2, faceR * 0.9f);

            g.setColour(Colours::black.withAlpha(0.5f));
            g.drawEllipse(centre.x - faceR, centre.y - faceR, faceR * 2, faceR * 2, 2.0f);
        }

        // THE CROSS — glowing, rotating with the value
        {
            const float ch = faceR * 1.18f, cw = ch * 0.62f;
            juce::Rectangle<float> markBounds(centre.x - cw * 0.5f, centre.y - ch * 0.52f, cw, ch);
            const auto rot = AffineTransform::rotation(angle * 0.85f, centre.x, centre.y);

            Graphics::ScopedSaveState save(g);
            g.addTransform(rot);

            // bloom passes
            for (float pass : { 7.0f, 3.5f })
            {
                Path cross;
                const float t = cw * 0.30f;
                const float crossbarY = markBounds.getY() + ch * 0.62f;
                cross.addRoundedRectangle(centre.x - t * 0.5f, markBounds.getY(), t, ch, t * 0.35f);
                cross.addRoundedRectangle(markBounds.getX(), crossbarY - t * 0.5f, cw, t, t * 0.35f);
                g.setColour(led.withAlpha(pass > 5.0f ? 0.16f + 0.10f * glowLevel : 0.30f));
                g.strokePath(cross, PathStrokeType(pass));
            }

            drawInfernalLoveMark(g, markBounds,
                                 led.brighter(0.18f + 0.25f * glowLevel),
                                 Colour(0xff141416));
        }

        // the wide LED halo ring
        drawDottedArc(g, centre, outer * 0.93f, value01, 27, outer * 0.034f, led);
    }

    //==========================================================================
    // VU: dark smoked glass, the cross watermarked behind the scale, pink
    // needle sweep — the shelved design's meter, lamp riding the level.
    inline void drawVUMeter(Graphics& g, juce::Rectangle<float> bounds, float value01, float glow01 = 0.0f)
    {
        auto r = bounds.reduced(2.0f);
        glow01 = jlimit(0.0f, 1.0f, glow01);

        g.setColour(Colours::black.withAlpha(0.5f));
        g.fillRoundedRectangle(r.translated(0, 2.0f).expanded(1.5f), 8.0f);

        ColourGradient bezel(Colour(0xff424247), r.getCentreX(), r.getY(),
                             Colour(0xff101012), r.getCentreX(), r.getBottom(), false);
        g.setGradientFill(bezel);
        g.fillRoundedRectangle(r, 8.0f);

        auto face = r.reduced(4.0f);
        ColourGradient smoke(Colour(0xff1b1b1f), face.getCentreX(), face.getY(),
                             Colour(0xff0a0a0c), face.getCentreX(), face.getBottom(), false);
        g.setGradientFill(smoke);
        g.fillRoundedRectangle(face, 5.0f);

        {
            Graphics::ScopedSaveState save(g);
            Path clip; clip.addRoundedRectangle(face, 5.0f);
            g.reduceClipRegion(clip);

            // the watermark cross
            const float wmH = face.getHeight() * 0.62f;
            drawInfernalLoveMark(g, { face.getCentreX() - wmH * 0.31f, face.getCentreY() - wmH * 0.42f,
                                      wmH * 0.62f, wmH },
                                 Colours::white.withAlpha(0.07f), Colour(0xff121215));

            // lamp rides the audio
            ColourGradient lamp(accent.withAlpha(0.06f + 0.22f * glow01),
                                face.getCentreX(), face.getBottom(),
                                accent.withAlpha(0.0f), face.getCentreX(), face.getY(), false);
            g.setGradientFill(lamp);
            g.fillRoundedRectangle(face, 5.0f);

            fillGrunge(g, face, 0.5f);
        }

        const auto pivot = Point<float>(face.getCentreX(), face.getBottom() + face.getHeight() * 0.55f);
        const float needleLen = face.getHeight() * 1.32f;
        const float minAngle = -0.62f, maxAngle = 0.62f;

        // scale: ivory ticks with dB labels, red zone on the right
        static const float marks[] = { -20.0f, -10.0f, -7.0f, -5.0f, -3.0f, -1.0f, 0.0f, 1.0f, 2.0f, 3.0f };
        for (float dB : marks)
        {
            const float frac = (dB + 20.0f) / 23.0f;
            const float a = minAngle + (maxAngle - minAngle) * frac;
            const bool red = dB >= 0.0f;
            const auto p1 = pivot.getPointOnCircumference(needleLen, a);
            const auto p2 = pivot.getPointOnCircumference(needleLen - 5.0f, a);
            g.setColour(red ? accent : Colours::white.withAlpha(0.55f));
            g.drawLine({ p1, p2 }, red ? 1.6f : 1.1f);

            const auto pl = pivot.getPointOnCircumference(needleLen + 6.0f, a);
            g.setColour(red ? accent.withAlpha(0.9f) : Colours::white.withAlpha(0.45f));
            g.setFont(labelFont(6.5f));
            g.drawText(dB > 0 ? "+" + String((int) dB) : String((int) dB),
                       (int) pl.x - 9, (int) pl.y - 5, 18, 9, Justification::centred, false);
        }

        {
            Path red;
            const float a0 = minAngle + (maxAngle - minAngle) * (20.0f / 23.0f);
            red.addCentredArc(pivot.x, pivot.y, needleLen + 1.0f, needleLen + 1.0f, 0.0f, a0, maxAngle, true);
            Graphics::ScopedSaveState save(g);
            Path clip; clip.addRoundedRectangle(face, 5.0f);
            g.reduceClipRegion(clip);
            g.setColour(accent.withAlpha(0.5f + 0.3f * glow01));
            g.strokePath(red, PathStrokeType(2.4f));
        }

        // needle
        const float needleAngle = minAngle + (maxAngle - minAngle) * jlimit(0.0f, 1.0f, value01);
        {
            Graphics::ScopedSaveState save(g);
            Path clip; clip.addRoundedRectangle(face, 5.0f);
            g.reduceClipRegion(clip);

            const auto tip = pivot.getPointOnCircumference(needleLen - 2.0f, needleAngle);
            g.setColour(accent.withAlpha(0.22f + 0.30f * glow01));
            g.drawLine(Line<float>(pivot, tip), 4.0f);
            g.setColour(Colour(0xffd8d8dc));
            g.drawLine(Line<float>(pivot, tip), 1.5f);
        }

        // glass
        {
            Graphics::ScopedSaveState save(g);
            Path clip; clip.addRoundedRectangle(face, 5.0f);
            g.reduceClipRegion(clip);
            ColourGradient glass(Colours::white.withAlpha(0.10f), face.getX(), face.getY(),
                                 Colours::transparentWhite, face.getX() + face.getWidth() * 0.5f,
                                 face.getY() + face.getHeight() * 0.8f, false);
            g.setGradientFill(glass);
            g.fillRect(face.withTrimmedBottom(face.getHeight() * 0.5f));
        }

        g.setColour(Colours::white.withAlpha(0.08f));
        g.drawRoundedRectangle(r.reduced(0.7f), 7.0f, 1.0f);
        drawScrew(g, { r.getX() + 6.0f, r.getY() + 6.0f }, 2.4f, 0.6f);
        drawScrew(g, { r.getRight() - 6.0f, r.getY() + 6.0f }, 2.4f, 2.2f);
        drawScrew(g, { r.getX() + 6.0f, r.getBottom() - 6.0f }, 2.4f, 1.1f);
        drawScrew(g, { r.getRight() - 6.0f, r.getBottom() - 6.0f }, 2.4f, 2.9f);
    }

    //==========================================================================
    // Large-format NAB reel, shelved-design colourway: bright silver flange
    // with three big rounded windows, wine-dark chrome-banded hub.
    inline void drawReel(Graphics& g, Point<float> centre, float radius, float angleRad,
                         float packAmount = 0.8f)
    {
        const float R = radius;
        auto discBounds = juce::Rectangle<float>(centre.x - R, centre.y - R, R * 2, R * 2);

        dropShadowEllipse(g, discBounds, 0.8f);

        // backplate + tape pack behind the flange
        g.setColour(Colour(0xff070708));
        g.fillEllipse(discBounds.reduced(R * 0.04f));

        const float packR = R * jmap(jlimit(0.0f, 1.0f, packAmount), 0.36f, 0.90f);
        ColourGradient pack(Colour(0xff181210), centre.x, centre.y - packR,
                            Colour(0xff0b0908), centre.x, centre.y + packR, false);
        g.setGradientFill(pack);
        g.fillEllipse(centre.x - packR, centre.y - packR, packR * 2, packR * 2);
        for (int i = 1; i <= 6; ++i)
        {
            const float gr = packR * (0.42f + 0.095f * (float) i);
            g.setColour(Colours::white.withAlpha(0.02f + 0.005f * (float) i));
            g.drawEllipse(centre.x - gr, centre.y - gr, gr * 2, gr * 2, 0.7f);
        }

        // silver flange minus three rounded windows
        Path flange;
        flange.setUsingNonZeroWinding(false);
        flange.addEllipse(discBounds.reduced(R * 0.015f));

        const float winOuter = R * 0.84f, winInner = R * 0.34f;
        const float halfSpan = 0.86f;
        Path windows;
        for (int s = 0; s < 3; ++s)
        {
            const float a = angleRad + (float) s * MathConstants<float>::twoPi / 3.0f;
            Path window;
            window.addCentredArc(centre.x, centre.y, winOuter, winOuter, a, -halfSpan, halfSpan, true);
            window.addCentredArc(centre.x, centre.y, winInner, winInner, a, halfSpan, -halfSpan, false);
            window.closeSubPath();
            window = window.createPathWithRoundedCorners(R * 0.14f);
            flange.addPath(window);
            windows.addPath(window);
        }

        {
            Graphics::ScopedSaveState save(g);
            g.reduceClipRegion(flange);

            ColourGradient metal(silverHi, centre.x - R * 0.3f, centre.y - R,
                                 silverLo, centre.x + R * 0.2f, centre.y + R, false);
            g.setGradientFill(metal);
            g.fillEllipse(discBounds);

            for (int i = 0; i < 6; ++i)
            {
                const float rr = R * (0.40f + 0.095f * (float) i);
                g.setColour(Colours::black.withAlpha(0.07f));
                g.drawEllipse(centre.x - rr, centre.y - rr, rr * 2, rr * 2, 0.6f);
            }

            ColourGradient spec(Colours::white.withAlpha(0.30f), centre.x - R * 0.6f, centre.y - R * 0.9f,
                                Colours::transparentWhite, centre.x + R * 0.1f, centre.y, false);
            g.setGradientFill(spec);
            g.fillRect(discBounds.withHeight(R * 1.0f));

            fillGrunge(g, discBounds, 0.35f);
        }

        g.setColour(Colours::black.withAlpha(0.5f));
        g.strokePath(windows, PathStrokeType(1.6f));
        g.setColour(Colours::white.withAlpha(0.25f));
        g.strokePath(windows, PathStrokeType(0.7f), AffineTransform::translation(0.0f, 1.0f));

        g.setColour(Colours::white.withAlpha(0.35f));
        g.drawEllipse(discBounds.reduced(R * 0.015f), 1.4f);
        g.setColour(Colours::black.withAlpha(0.55f));
        g.drawEllipse(discBounds, 1.2f);

        // wine-dark hub with chrome bands
        const float hubR = R * 0.27f;
        {
            dropShadowEllipse(g, { centre.x - hubR, centre.y - hubR, hubR * 2, hubR * 2 }, 0.5f);

            // chrome band
            ColourGradient chrome(Colour(0xffe6e6ea), centre.x, centre.y - hubR,
                                  Colour(0xff5b5b62), centre.x, centre.y + hubR, false);
            g.setGradientFill(chrome);
            g.fillEllipse(centre.x - hubR, centre.y - hubR, hubR * 2, hubR * 2);

            // wine cylinder
            const float wr = hubR * 0.86f;
            ColourGradient wine(wineHub.brighter(0.35f), centre.x - wr * 0.4f, centre.y - wr * 0.6f,
                                Colour(0xff1d0a11), centre.x + wr * 0.3f, centre.y + wr, true);
            g.setGradientFill(wine);
            g.fillEllipse(centre.x - wr, centre.y - wr, wr * 2, wr * 2);

            // glassy highlight across the wine
            ColourGradient gloss(Colours::white.withAlpha(0.28f), centre.x - wr * 0.5f, centre.y - wr * 0.7f,
                                 Colours::transparentWhite, centre.x, centre.y + wr * 0.1f, false);
            g.setGradientFill(gloss);
            g.fillEllipse(centre.x - wr, centre.y - wr, wr * 2, wr * 0.95f);

            // inner chrome ring + dark spindle
            g.setColour(Colour(0xffcfcfd4).withAlpha(0.8f));
            g.drawEllipse(centre.x - wr * 0.55f, centre.y - wr * 0.55f, wr * 1.1f, wr * 1.1f, 1.6f);
            g.setColour(Colour(0xff0a0a0b));
            g.fillEllipse(centre.x - wr * 0.16f, centre.y - wr * 0.16f, wr * 0.32f, wr * 0.32f);
            g.setColour(Colours::white.withAlpha(0.5f));
            g.fillEllipse(centre.x - wr * 0.05f, centre.y - wr * 0.22f, wr * 0.1f, wr * 0.1f);
        }
    }

    //==========================================================================
    // Pills and lamps (PRINT THROUGH, BYPASS, tape cross-lamp).
    inline void drawButton(Graphics& g, juce::Rectangle<float> bounds, const String& label,
                           bool on, bool pressed, bool hover = false)
    {
        auto r = bounds.reduced(1.5f);
        const float corner = r.getHeight() * 0.5f;

        if (! pressed)
        {
            g.setColour(Colours::black.withAlpha(0.45f));
            g.fillRoundedRectangle(r.translated(0, 1.5f), corner);
        }

        // recessed channel
        ColourGradient channel(Colour(0xff0c0c0e), r.getCentreX(), r.getY(),
                               Colour(0xff1c1c1f), r.getCentreX(), r.getBottom(), false);
        g.setGradientFill(channel);
        g.fillRoundedRectangle(r, corner);
        g.setColour(Colours::white.withAlpha(hover ? 0.16f : 0.09f));
        g.drawRoundedRectangle(r, corner, 1.0f);

        // sliding dot
        const float dotR = r.getHeight() * 0.32f;
        const float dx = on ? r.getRight() - corner : r.getX() + corner;
        if (on)
        {
            g.setColour(accent.withAlpha(0.45f));
            g.fillEllipse(dx - dotR * 2.0f, r.getCentreY() - dotR * 2.0f, dotR * 4.0f, dotR * 4.0f);
        }
        ColourGradient dot(on ? accent.brighter(0.2f) : Colour(0xff5a5a60),
                           dx, r.getCentreY() - dotR,
                           on ? accent.darker(0.4f) : Colour(0xff2a2a2e),
                           dx, r.getCentreY() + dotR, false);
        g.setGradientFill(dot);
        g.fillEllipse(dx - dotR, r.getCentreY() - dotR, dotR * 2, dotR * 2);
        g.setColour(Colours::white.withAlpha(0.5f));
        g.fillEllipse(dx - dotR * 0.4f, r.getCentreY() - dotR * 0.6f, dotR * 0.7f, dotR * 0.7f);

        ignoreUnused(label);
    }

    //==========================================================================
    // Distressed-chrome wordmark, two lines: THE KISS OF / SHAME.
    inline void drawWordmark(Graphics& g, juce::Rectangle<float> r)
    {
        auto top = r.removeFromTop(r.getHeight() * 0.42f);

        auto chrome = [&](const String& text, juce::Rectangle<float> area, float height)
        {
            g.setColour(Colours::black.withAlpha(0.65f));
            g.setFont(labelFont(height));
            g.drawText(text, area.translated(0.0f, 1.6f), Justification::centred, false);

            ColourGradient steel(Colour(0xfff2f2f5), area.getCentreX(), area.getY(),
                                 Colour(0xff8b8b92), area.getCentreX(), area.getBottom(), false);
            steel.addColour(0.48, Colours::white);
            steel.addColour(0.55, Colour(0xff9d9da4));
            g.setGradientFill(steel);
            g.drawText(text, area, Justification::centred, false);
        };

        chrome("THE KISS OF", top, top.getHeight() * 0.78f);
        chrome("SHAME", r, r.getHeight() * 0.97f);
    }
}
