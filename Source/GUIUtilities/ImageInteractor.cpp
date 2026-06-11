#include "ImageInteractor.h"

ImageInteractor::ImageInteractor() = default;

void ImageInteractor::setNumFrames(int newNumFrames)
{
    curValue = 0;
    numFrames = jmax(1, newNumFrames);
}

void ImageInteractor::setAnimationImage(const Image& newImage)
{
    satImage = newImage;
    desatImage = newImage.createCopy();
    desatImage.desaturate();
    repaint();
}

void ImageInteractor::setDimensions(int topLeftX, int topLeftY, int w, int h)
{
    setTopLeftPosition(topLeftX, topLeftY);
    frameWidth = w;
    frameHeight = h;
    setSize(frameWidth, frameHeight);
}

void ImageInteractor::paint(Graphics& g)
{
    if (era == UIEra::modern)
    {
        if (modernStyle == ModernStyle::vuMeter)
            ModernTheme::drawVUMeter(g, getLocalBounds().toFloat(), getNormalizedValue(),
                                     getNormalizedValue());

        return; // hidden: the vector era draws its own world
    }

    const Image& image = desaturate ? desatImage : satImage;

    if (! image.isNull())
    {
        const int frameNum = (int) (getNormalizedValue() * (numFrames - 1));
        juce::Rectangle<int> clipRect(0, frameNum * frameHeight, frameWidth, frameHeight);
        g.drawImageAt(image.getClippedImage(clipRect), 0, 0);
    }
}
