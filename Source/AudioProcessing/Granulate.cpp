/***************************************************/
/*! \class Granulate
    \brief STK granular synthesis class.

    by Gary Scavone, 2005 - 2010.
    See Granulate.h for the Rev 2 modernization notes.
*/
/***************************************************/

#include "Granulate.h"
#include <cmath>

Granulate::Granulate()
{
    setGrainParameters();
    setRandomFactor();
    lastFrame.resize(2, 0.0f);
}

void Granulate::setAudioData(const AudioBuffer<float>& data)
{
    audioData = data;
    lastFrame.assign((size_t) jmax(1, audioData.getNumChannels()), 0.0f);
    reset();
}

void Granulate::setSampleRate(double newSampleRate)
{
    sampleRate = newSampleRate;
}

void Granulate::setStretch(unsigned int stretchFactor)
{
    if (stretchFactor <= 1)
        gStretch_ = 0;
    else if (gStretch_ >= 1000)
        gStretch_ = 1000;
    else
        gStretch_ = stretchFactor - 1;
}

void Granulate::setGrainParameters(unsigned int duration, unsigned int rampPercent,
                                   int offset, unsigned int delay)
{
    gDuration_ = jmax(1u, duration);
    gRampPercent_ = jmin(100u, rampPercent);
    gOffset_ = offset;
    gDelay_ = delay;
}

void Granulate::setRandomFactor(float randomness)
{
    gRandomFactor_ = 0.97f * jlimit(0.0f, 1.0f, randomness);
}

void Granulate::reset()
{
    gPointer_ = 0;

    const size_t nVoices = grains_.size();
    for (size_t i = 0; i < nVoices; ++i)
    {
        grains_[i].repeats = 0;
        grains_[i].counter = (unsigned long) (i * gDuration_ * 0.001 * sampleRate / (double) nVoices);
        grains_[i].state = GRAIN_STOPPED;
    }

    std::fill(lastFrame.begin(), lastFrame.end(), 0.0f);
}

void Granulate::setVoices(unsigned int nVoices)
{
    const size_t oldSize = grains_.size();
    grains_.resize(nVoices);

    for (size_t i = oldSize; i < nVoices; ++i)
    {
        grains_[i].repeats = 0;
        grains_[i].counter = (unsigned long) (i * gDuration_ * 0.001 * sampleRate / (double) nVoices);
        grains_[i].pointer = gPointer_;
        grains_[i].state = GRAIN_STOPPED;
    }

    gain_ = grains_.empty() ? 1.0f : 1.0f / (float) grains_.size();
}

void Granulate::calculateGrain(Granulate::Grain& grain)
{
    if (grain.repeats > 0)
    {
        grain.repeats--;
        grain.pointer = (float) grain.startPointer;
        if (grain.attackCount > 0)
        {
            grain.eScaler = 0.0f;
            grain.eRate = -grain.eRate;
            grain.counter = grain.attackCount;
            grain.state = GRAIN_FADEIN;
        }
        else
        {
            grain.counter = grain.sustainCount;
            grain.state = GRAIN_SUSTAIN;
        }
        return;
    }

    // Calculate duration and envelope parameters.
    float seconds = gDuration_ * 0.001f;
    seconds += seconds * gRandomFactor_ * noise.tick();
    unsigned long count = (unsigned long) (seconds * sampleRate);
    grain.attackCount = (unsigned int) (gRampPercent_ * 0.005 * count);
    grain.decayCount = grain.attackCount;
    grain.sustainCount = count - 2 * grain.attackCount;
    grain.eScaler = 0.0f;
    if (grain.attackCount > 0)
    {
        grain.eRate = 1.0f / grain.attackCount;
        grain.counter = grain.attackCount;
        grain.state = GRAIN_FADEIN;
    }
    else
    {
        grain.counter = grain.sustainCount;
        grain.state = GRAIN_SUSTAIN;
    }

    // Calculate delay parameter.
    seconds = gDelay_ * 0.001f;
    seconds += seconds * gRandomFactor_ * noise.tick();
    grain.delayCount = (unsigned long) (seconds * sampleRate);

    // Save stretch parameter.
    grain.repeats = gStretch_;

    // Calculate offset parameter.
    seconds = gOffset_ * 0.001f;
    seconds += seconds * gRandomFactor_ * std::abs(noise.tick());
    int offset = (int) (seconds * sampleRate);

    // Add some randomization to the pointer start position.
    seconds = gDuration_ * 0.001f * gRandomFactor_ * noise.tick();
    offset += (int) (seconds * sampleRate);
    grain.pointer += (float) offset;

    const int numSamples = audioData.getNumSamples();
    if (numSamples > 0)
    {
        while (grain.pointer >= (float) numSamples)
            grain.pointer -= (float) numSamples;
    }

    if (grain.pointer < 0)
        grain.pointer = 0;
    grain.startPointer = (unsigned long) grain.pointer;
}

float Granulate::tick(unsigned int channel)
{
    const unsigned int nChannels = (unsigned int) jmin((int) lastFrame.size(), audioData.getNumChannels());

    for (size_t j = 0; j < lastFrame.size(); ++j)
        lastFrame[j] = 0.0f;

    if (audioData.getNumSamples() == 0 || channel >= lastFrame.size())
        return 0.0f;

    float sample;
    for (size_t i = 0; i < grains_.size(); ++i)
    {
        if (grains_[i].counter == 0) // Update the grain state.
        {
            switch (grains_[i].state)
            {
            case GRAIN_STOPPED:
                // We're done waiting between grains ... setup for new grain
                calculateGrain(grains_[i]);
                break;

            case GRAIN_FADEIN:
                // We're done ramping up the envelope
                if (grains_[i].sustainCount > 0)
                {
                    grains_[i].counter = grains_[i].sustainCount;
                    grains_[i].state = GRAIN_SUSTAIN;
                    break;
                }
                // else no sustain state (i.e. perfect triangle window)
                JUCE_FALLTHROUGH;

            case GRAIN_SUSTAIN:
                // We're done with flat part of envelope ... setup to ramp down
                if (grains_[i].decayCount > 0)
                {
                    grains_[i].counter = grains_[i].decayCount;
                    grains_[i].eRate = -grains_[i].eRate;
                    grains_[i].state = GRAIN_FADEOUT;
                    break;
                }
                // else no fade out state (gRampPercent = 0)
                JUCE_FALLTHROUGH;

            case GRAIN_FADEOUT:
                // We're done ramping down ... setup for wait between grains
                if (grains_[i].delayCount > 0)
                {
                    grains_[i].counter = grains_[i].delayCount;
                    grains_[i].state = GRAIN_STOPPED;
                    break;
                }
                // else no delay (gDelay = 0)
                calculateGrain(grains_[i]);
            }
        }

        // Accumulate the grain outputs.
        if (grains_[i].state > 0)
        {
            const int pointerIndex = jlimit(0, audioData.getNumSamples() - 1, (int) grains_[i].pointer);

            for (unsigned int j = 0; j < nChannels; ++j)
            {
                sample = audioData.getReadPointer((int) j)[pointerIndex];

                if (grains_[i].state == GRAIN_FADEIN || grains_[i].state == GRAIN_FADEOUT)
                {
                    sample *= grains_[i].eScaler;
                    grains_[i].eScaler += grains_[i].eRate;
                }
                lastFrame[j] += sample;
            }

            // Increment and check pointer limits.
            grains_[i].pointer++;
            if (grains_[i].pointer >= (float) audioData.getNumSamples())
                grains_[i].pointer = 0;
        }

        // Decrement counter for all states.
        if (grains_[i].counter > 0)
            grains_[i].counter--;
    }

    // Increment our global file pointer at the stretch rate.
    if (stretchCounter_++ == gStretch_)
    {
        gPointer_++;
        if (gPointer_ >= (float) audioData.getNumSamples())
            gPointer_ = 0;
        stretchCounter_ = 0;
    }

    return lastFrame[channel];
}
