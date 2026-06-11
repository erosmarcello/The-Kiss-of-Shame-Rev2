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
        if (fps > 0)
            startTimerHz(fps);
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
        const float dragDist = (float) event.getDistanceFromDragStartY() / 100.0f;
        curFlangeDepth = jlimit(0.0f, 1.0f, setFlangeDepth + dragDist);

        if (onFlangeDepthChanged != nullptr)
            onFlangeDepthChanged(curFlangeDepth);
    }

    float getCurrentFlangeDepth() const { return curFlangeDepth; }

    std::function<void()> onFlangeGestureStart;
    std::function<void(float)> onFlangeDepthChanged;
    std::function<void()> onFlangeGestureEnd;

    void setEra(UIEra newEra)
    {
        era = newEra;
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

        g.fillAll(ModernTheme::panelDeep);

        // The same rotation the filmstrip would show, derived from the frame
        // counter so transport/drag behavior is identical in both eras.
        const float angle = (float) currentFrame * MathConstants<float>::twoPi / (float) jmax(1, animationNumFrames);

        const Point<float> leftHub(r.getWidth() * 0.26f, r.getHeight() * 0.47f);
        const Point<float> rightHub(r.getWidth() * 0.74f, r.getHeight() * 0.47f);
        const float radius = r.getHeight() * 0.40f;

        // tape path between the reels
        g.setColour(ModernTheme::outline);
        g.drawLine(leftHub.x, leftHub.y + radius * 0.92f, rightHub.x, rightHub.y + radius * 0.92f, 2.0f);

        ModernTheme::drawReel(g, leftHub, radius, angle);
        ModernTheme::drawReel(g, rightHub, radius, -angle);

        g.setColour(ModernTheme::textDim);
        g.setFont(ModernTheme::labelFont(10.0f));
        g.drawText("DRAG THE REELS TO LEAN ON THE TAPE",
                   r.removeFromBottom(18.0f), Justification::centred, false);
    }

    void timerCallback() override
    {
        // The increment/threshold gate slows the reels while they're being
        // touched — preserved from the original update() logic.
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

        repaint();
    }

    UIEra era = UIEra::heritage;

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
