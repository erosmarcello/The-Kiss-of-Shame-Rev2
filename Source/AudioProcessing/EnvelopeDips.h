#pragma once

#include "../shameConfig.h"

// Looping random amplitude-dip envelope: each pass through the domain
// scatters a fresh set of dip points — the slow "breathing" of worn tape.
// Rev 2: sample-rate aware, juce::Random instead of srand/rand, preallocated
// point storage so the per-loop recalculation never allocates on the audio
// thread, and interpolation guards against degenerate spans.
class EnvelopeDips
{
public:
    EnvelopeDips()
    {
        points.ensureStorageAllocated(64);
        prepare(44100.0);
        calculateDipPoints();
    }

    void prepare(double sampleRate)
    {
        samplesPerMs = (float) (sampleRate / 1000.0);
        domain = domainMS * samplesPerMs;
        incr = 0.0f;
    }

    void calculateDipPoints()
    {
        points.clearQuick();

        const float startingValue = 1.0f;
        const int numRandPoints = jmax(1, (int) (numPoints * (1.0f - numPointRandomness * random.nextFloat()) + 1));
        const float partitionSize = 1.0f / ((float) numRandPoints + 1.0f);

        points.add({ 0.0f, startingValue });
        for (int i = 0; i < numRandPoints; ++i)
        {
            const float xInit = ((float) i + 1.0f) / (float) (numRandPoints + 1);
            float xDeviation = random.nextFloat() * partitionSize / 2.5f;
            if (random.nextBool())
                xDeviation = -xDeviation;

            points.add({ jlimit(0.001f, 0.999f, xInit + xDeviation),
                         1.0f - dynamicExtremity * random.nextFloat() });
        }
        points.add({ 1.0f, startingValue });
    }

    void setDomainMS(float d)
    {
        domainMS = d;
        domain = jmax(1.0f, domainMS * samplesPerMs);
    }

    void setDynamicExtremity(float dE)   { dynamicExtremity = jlimit(0.0f, 1.0f, dE); }
    void setNumPoints(int nP)            { numPoints = jlimit(1, 40, nP); }
    void setNumPointRandomness(float r)  { numPointRandomness = jlimit(0.0f, 1.0f, r); }

    float processEnvelopeDips()
    {
        const float curPos = jlimit(0.0f, 1.0f, incr / domain);

        float priorX = points[0].getX(), priorY = points[0].getY();
        float nxtX = points[points.size() - 1].getX(), nxtY = points[points.size() - 1].getY();

        for (int i = points.size() - 1; i >= 0; --i)
        {
            if (curPos >= points[i].getX())
            {
                priorX = points[i].getX();
                priorY = points[i].getY();
                if (i + 1 < points.size())
                {
                    nxtX = points[i + 1].getX();
                    nxtY = points[i + 1].getY();
                }
                else
                {
                    nxtX = priorX;
                    nxtY = priorY;
                }
                break;
            }
        }

        const float span = nxtX - priorX;
        const float distFromPrior = span > 0.0f ? (curPos - priorX) / span : 0.0f;
        const float interpolatedValue = (1.0f - distFromPrior) * priorY + distFromPrior * nxtY;

        incr++;
        if (incr >= domain)
        {
            calculateDipPoints();
            incr = 0;
        }

        return interpolatedValue;
    }

private:
    float samplesPerMs = 44.1f;
    float domainMS = 1000.0f;

    float incr = 0.0f;
    float domain = 44100.0f;
    float dynamicExtremity = 0.0f;
    float numPointRandomness = 0.0f;
    int numPoints = 5;

    Random random;
    Array<Point<float>> points;
};
