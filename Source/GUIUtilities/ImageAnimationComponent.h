//
//  ImageAnimationComponent.h
//  KissOfShame
//
//  Created by Brian Hansen on 9/4/14.
//  Rev 2: plain Timer-driven Component (the AnimatedAppComponent +
//  OpenGLContext stack is gone, along with the 1-pixel seam workaround it
//  required). Flange gestures are reported through std::function callbacks
//  instead of ActionBroadcaster strings.
//
//  These are the interactive reels: they spin with the transport, and
//  click-dragging them leans on the tape for a real-time flange.
//

#pragma once

#include "Theme.h"

class ImageAnimationComponent : public Component, private Timer
{
public:
    ImageAnimationComponent(const Image& image, int numFrames, int framesPerSecond)
        : animationNumFrames(jmax(1, numFrames))
    {
        animationImage = image;
        imageFrameWidth = animationImage.getWidth();
        imageFrameHeight = animationImage.getHeight() / animationNumFrames;
        setSize(imageFrameWidth, imageFrameHeight);

        endFrame = animationNumFrames - 1;

        setFramesPerSecond(framesPerSecond);
    }

    ~ImageAnimationComponent() override = default;

    //==========================================================================
    void setFramesPerSecond(int fps)
    {
        spinning = fps > 0;

        if (spinning)
            startTimerHz(fps);
        else if (era == UIEra::modern)
            startTimerHz(20); // idle shimmer keeps the modern bay alive
        else
            stopTimer();
    }

    void mouseDown(const MouseEvent&) override
    {
        // Touching the reels drags them: animation slows against the thumb.
        setAnimationResetThreshold(0.015f);
        if (onFlangeGestureStart != nullptr)
            onFlangeGestureStart();
    }

    void mouseUp(const MouseEvent&) override
    {
        setFlangeDepth = curFlangeDepth;
        setAnimationResetThreshold(0.0f);
        if (onFlangeGestureEnd != nullptr)
            onFlangeGestureEnd();
    }

    void mouseDrag(const MouseEvent& event) override
    {
        // 200 px of vertical travel sweeps the full flange depth, matching the
        // web demo; dragging down leans harder on the tape.
        const float dragDist = (float) event.getDistanceFromDragStartY() / 200.0f;
        curFlangeDepth = jlimit(0.0f, 1.0f, setFlangeDepth + dragDist);

        if (onFlangeDepthChanged != nullptr)
            onFlangeDepthChanged(curFlangeDepth);
    }

    void mouseDoubleClick(const MouseEvent&) override
    {
        // Double-click the reels to let go of the tape cleanly (flange to
        // zero), exactly like the web demo.
        curFlangeDepth = 0.0f;
        setFlangeDepth = 0.0f;

        if (onFlangeGestureStart != nullptr) onFlangeGestureStart();
        if (onFlangeDepthChanged != nullptr) onFlangeDepthChanged(0.0f);
        if (onFlangeGestureEnd != nullptr)   onFlangeGestureEnd();
    }

    float getCurrentFlangeDepth() const { return curFlangeDepth; }

    std::function<void()> onFlangeGestureStart;
    std::function<void(float)> onFlangeDepthChanged;
    std::function<void()> onFlangeGestureEnd;

    void setEra(UIEra newEra)
    {
        era = newEra;

        if (era == UIEra::modern && ! spinning)
            startTimerHz(20);
        else if (era == UIEra::heritage && ! spinning)
            stopTimer();

        repaint();
    }

    //==========================================================================
    void paint(Graphics& g) override
    {
        if (era == UIEra::modern)
        {
            paintModernReels(g);
            return;
        }

        g.fillAll(Colours::black);

        if (! animationImage.isNull())
        {
            juce::Rectangle<int> clipRect(0, currentFrame * imageFrameHeight, imageFrameWidth, imageFrameHeight);
            g.drawImageAt(animationImage.getClippedImage(clipRect), 0, 0);
        }
    }

    void setFrameDimensions(int topLeftX, int topLeftY, int w, int h)
    {
        setTopLeftPosition(topLeftX, topLeftY);
        imageFrameWidth = w;
        imageFrameHeight = h;
        setSize(imageFrameWidth, imageFrameHeight);
    }

    void setStartingFrame(int frameNumber)
    {
        startFrame = frameNumber;
        if (currentFrame < startFrame) currentFrame = startFrame;
    }

    void setEndingFrame(int frameNumber)            { endFrame = frameNumber; }
    void setAnimationRate(float rate)               { incrRate = rate; }
    void setAnimationResetThreshold(float thresh)   { resetThresh = thresh; }

    bool isAnimating = false;

private:
    void paintModernReels(Graphics& g)
    {
        auto r = getLocalBounds().toFloat();

        // transport bay: vertical light falloff + vignette + grain
        ColourGradient bay(ModernTheme::panel.brighter(0.02f), r.getCentreX(), r.getY(),
                           ModernTheme::panelDeep, r.getCentreX(), r.getBottom(), false);
        g.setGradientFill(bay);
        g.fillAll();

        ColourGradient vignette(Colours::transparentBlack, r.getCentreX(), r.getCentreY(),
                                Colours::black.withAlpha(0.45f), r.getX(), r.getY(), true);
        vignette.addColour(0.62, Colours::transparentBlack);
        g.setGradientFill(vignette);
        g.fillRect(r);

        ModernTheme::fillGrain(g, r, 0.5f);

        // The same rotation the filmstrip would show, derived from the frame
        // counter so transport/drag behavior is identical in both eras.
        const float angle = (float) currentFrame * MathConstants<float>::twoPi / (float) jmax(1, animationNumFrames);

        const Point<float> leftHub(r.getWidth() * 0.255f, r.getHeight() * 0.455f);
        const Point<float> rightHub(r.getWidth() * 0.745f, r.getHeight() * 0.455f);
        const float radius = r.getHeight() * 0.44f; // large-format 10.5" presence
        const float tapeY = leftHub.y + radius * 0.96f;

        // tape path: dark band with a moving sheen, threaded under both reels
        {
            g.setColour(Colour(0xff060607));
            g.fillRect(leftHub.x, tapeY - 2.5f, rightHub.x - leftHub.x, 5.0f);
            g.setColour(Colours::white.withAlpha(0.10f));
            g.drawLine(leftHub.x, tapeY - 2.5f, rightHub.x, tapeY - 2.5f, 0.8f);

            // a glint travels down the tape while the transport runs
            if (spinning)
            {
                const float span = rightHub.x - leftHub.x;
                const float gx = leftHub.x + std::fmod(shimmerPhase / MathConstants<float>::twoPi, 1.0f) * span;
                ColourGradient glint(Colours::white.withAlpha(0.30f), gx, tapeY,
                                     Colours::transparentWhite, gx + 34.0f, tapeY, true);
                g.setGradientFill(glint);
                g.fillRect(gx - 34.0f, tapeY - 2.5f, 68.0f, 5.0f);
            }

            // guide rollers
            for (float gx : { leftHub.x + radius * 0.55f, rightHub.x - radius * 0.55f })
            {
                ColourGradient roller(Colour(0xff4a4a50), gx, tapeY - 7.0f, Colour(0xff19191b), gx, tapeY + 7.0f, false);
                g.setGradientFill(roller);
                g.fillEllipse(gx - 5.5f, tapeY - 5.5f, 11.0f, 11.0f);
                g.setColour(Colours::black.withAlpha(0.6f));
                g.drawEllipse(gx - 5.5f, tapeY - 5.5f, 11.0f, 11.0f, 1.0f);
                g.setColour(Colours::white.withAlpha(0.25f));
                g.fillEllipse(gx - 1.5f, tapeY - 1.5f, 3.0f, 3.0f);
            }
        }

        // head block between the reels
        {
            juce::Rectangle<float> head(r.getCentreX() - 36.0f, tapeY - 26.0f, 72.0f, 30.0f);
            g.setColour(Colours::black.withAlpha(0.5f));
            g.fillRoundedRectangle(head.translated(0, 2.0f), 5.0f);
            ColourGradient headFill(Colour(0xff37373c), head.getCentreX(), head.getY(),
                                    Colour(0xff141416), head.getCentreX(), head.getBottom(), false);
            g.setGradientFill(headFill);
            g.fillRoundedRectangle(head, 5.0f);
            g.setColour(Colours::white.withAlpha(0.10f));
            g.drawLine(head.getX() + 4.0f, head.getY() + 1.0f, head.getRight() - 4.0f, head.getY() + 1.0f, 1.0f);

            // the gap windows
            g.setColour(Colour(0xff0a0a0b));
            for (int i = 0; i < 3; ++i)
                g.fillRoundedRectangle(head.getX() + 9.0f + 19.0f * (float) i, head.getY() + 8.0f, 14.0f, 14.0f, 2.0f);

            ModernTheme::drawScrew(g, { head.getX() + 6.0f, head.getBottom() - 6.0f }, 2.2f, 0.9f);
            ModernTheme::drawScrew(g, { head.getRight() - 6.0f, head.getBottom() - 6.0f }, 2.2f, 2.4f);
        }

        // supply reel runs full, take-up lean — a transport mid-spool
        ModernTheme::drawReel(g, leftHub, radius, angle, 0.85f);
        ModernTheme::drawReel(g, rightHub, radius, -angle, 0.48f);

        // drifting sheen across each flange — light moving on machined metal
        for (const auto& hub : { leftHub, rightHub })
        {
            Graphics::ScopedSaveState save(g);
            const float faceR = radius * 0.97f;
            Path clip;
            clip.addEllipse(hub.x - faceR, hub.y - faceR, faceR * 2, faceR * 2);
            g.reduceClipRegion(clip);

            const float drift = std::sin(shimmerPhase) * faceR * 0.45f;
            ColourGradient sheen(Colours::white.withAlpha(0.045f), hub.x + drift, hub.y - faceR,
                                 Colours::transparentWhite, hub.x + drift, hub.y + faceR * 0.1f, false);
            g.setGradientFill(sheen);
            g.fillRect(hub.x - faceR, hub.y - faceR, faceR * 2, faceR * 1.1f);
        }

        // pink ember rising from the deck below — the machine is alive,
        // breathing slowly even at rest
        const float breath = 0.5f + 0.5f * std::sin(shimmerPhase * 0.5f);
        ColourGradient ember(ModernTheme::accent.withAlpha((hovering ? 0.13f : 0.07f) + 0.05f * breath),
                             r.getCentreX(), r.getBottom() + 30.0f,
                             ModernTheme::accent.withAlpha(0.0f), r.getCentreX(), r.getBottom() - 90.0f, true);
        g.setGradientFill(ember);
        g.fillRect(r.withTop(r.getBottom() - 90.0f));

        g.setColour(hovering ? ModernTheme::textPrimary.withAlpha(0.8f) : ModernTheme::textDim.withAlpha(0.55f));
        g.setFont(ModernTheme::labelFont(9.5f));
        g.drawText("DRAG THE REELS TO LEAN ON THE TAPE",
                   r.removeFromBottom(16.0f), Justification::centred, false);
    }

    void mouseEnter(const MouseEvent&) override { hovering = true; repaint(); }
    void mouseExit(const MouseEvent&) override  { hovering = false; repaint(); }

    void timerCallback() override
    {
        if (spinning)
        {
            // The increment/threshold gate slows the reels while they're
            // being touched — preserved from the original update() logic.
            if (curIncrement == 0.0f)
            {
                if (! animationImage.isNull())
                {
                    if (currentFrame >= endFrame)
                        currentFrame = startFrame;
                    ++currentFrame;
                }
            }

            curIncrement += incrRate;
            if (curIncrement >= resetThresh)
                curIncrement = 0.0f;
        }

        shimmerPhase += spinning ? 0.11f : 0.035f;
        if (shimmerPhase > MathConstants<float>::twoPi)
            shimmerPhase -= MathConstants<float>::twoPi;

        repaint();
    }

    UIEra era = UIEra::heritage;
    bool hovering = false;
    bool spinning = false;
    float shimmerPhase = 0.0f;

    float resetThresh = 1.0f;
    float curIncrement = 0.0f;
    float incrRate = 0.01f;

    Image animationImage;
    int imageFrameWidth = 0;
    int imageFrameHeight = 0;

    float curFlangeDepth = 0.0f;
    float setFlangeDepth = 0.0f;

    int animationNumFrames = 1;
    int currentFrame = 0;
    int startFrame = 0;
    int endFrame = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ImageAnimationComponent)
};
