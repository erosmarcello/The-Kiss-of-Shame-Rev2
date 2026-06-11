#pragma once

#include <vector>
#include "Noise.h"
#include "../shameConfig.h"

/***************************************************/
/*! \class Granulate
    \brief STK granular synthesis class.

    This class implements a real-time granular synthesis algorithm
    that operates on an input soundfile.  Multi-channel files are
    supported.  Various functions are provided to allow control over
    voice and grain parameters.

    The functionality of this class is based on the program MacPod by
    Chris Rolfe and Damian Keller, though there are likely to be a
    number of differences in the actual implementation.

    by Gary Scavone, 2005 - 2010.

    Rev 2: sample-rate aware, embedded audio data (no file paths), and
    channel/empty-buffer guards on the tick path.
*/
/***************************************************/

class Granulate
{
public:
    Granulate();

    ~Granulate() = default;

    //! Provide the (already resampled) audio bed to granulate.
    void setAudioData(const AudioBuffer<float>& data);

    //! Update the rate used to translate millisecond grain settings.
    void setSampleRate(double newSampleRate);

    //! Reset the file pointer and all existing grains to the file start.
    void reset();

    //! Set the number of simultaneous grain "voices" to use.
    void setVoices(unsigned int nVoices = 1);

    //! Set the stretch factor used for grain playback (1 - 1000).
    void setStretch(unsigned int stretchFactor = 1);

    //! Set global grain parameters used to determine individual grain settings.
    void setGrainParameters(unsigned int duration = 30, unsigned int rampPercent = 50,
                            int offset = 0, unsigned int delay = 0);

    //! This factor is used when setting individual grain parameters (0.0 - 1.0).
    void setRandomFactor(float randomness = 0.1f);

    //! Compute one sample frame and return the specified channel value.
    float tick(unsigned int channel = 0);

    enum GrainState
    {
        GRAIN_STOPPED,
        GRAIN_FADEIN,
        GRAIN_SUSTAIN,
        GRAIN_FADEOUT
    };

protected:
    struct Grain
    {
        float eScaler = 0.0f;
        float eRate = 0.0f;
        unsigned long attackCount = 0;
        unsigned long sustainCount = 0;
        unsigned long decayCount = 0;
        unsigned long delayCount = 0;
        unsigned long counter = 0;
        float pointer = 0;
        unsigned long startPointer = 0;
        unsigned int repeats = 0;
        GrainState state = GRAIN_STOPPED;
    };

    void calculateGrain(Grain& grain);

    std::vector<float> lastFrame;
    AudioBuffer<float> audioData;
    std::vector<Grain> grains_;
    Noise noise;
    float gPointer_ = 0.0f;
    double sampleRate = 44100.0;

    // Global grain parameters.
    unsigned int gDuration_ = 30;
    unsigned int gRampPercent_ = 50;
    unsigned int gDelay_ = 0;
    unsigned int gStretch_ = 0;
    unsigned int stretchCounter_ = 0;
    int gOffset_ = 0;
    float gRandomFactor_ = 0.097f;
    float gain_ = 1.0f;
};
