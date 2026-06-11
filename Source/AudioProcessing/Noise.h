#pragma once

/***************************************************/
/*! \class Noise
    \brief STK noise generator.

    Originally by Perry R. Cook and Gary P. Scavone, 1995--2014.
    Rev 2: backed by juce::Random — no more global srand()/rand() state
    shared across plugin instances.
*/
/***************************************************/

#include "../shameConfig.h"

class Noise
{
public:
    explicit Noise(int64 seed = 0)
    {
        if (seed != 0)
            random.setSeed(seed);
    }

    float lastOut() const { return lastFrame; }

    float tick()
    {
        return lastFrame = random.nextFloat() * 2.0f - 1.0f;
    }

private:
    Random random;
    float lastFrame = 0.0f;
};
