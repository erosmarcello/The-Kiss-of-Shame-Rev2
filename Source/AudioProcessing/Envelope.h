#pragma once

#include "../shameConfig.h"

// Point-based looping envelope in the millisecond domain.
// Rev 2: sample-rate aware, and the interpolation no longer reads
// uninitialized point variables when curPos lands outside the point list.
class Envelope
{
public:
    explicit Envelope(int domainMS_)
        : domainMS((float) domainMS_), loopDurationMS((float) domainMS_)
    {
        points.add({ 0.0f, 0.0f });
        points.add({ 1.0f, 0.0f });
        prepare(44100.0);
    }

    void prepare(double sampleRate)
    {
        samplesPerMs = (float) (sampleRate / 1000.0);
        domain = domainMS * samplesPerMs;
        loopDuration = loopDurationMS * samplesPerMs;
        incr = 0.0f;
    }

    void addEvelopePoint(float x, float y)
    {
        points.insert(points.size() - 1, { x, y });
    }

    void setDomainMS(float d)
    {
        domainMS = d;
        domain = domainMS * samplesPerMs;
        if (domain > loopDuration) loopDuration = domain;
    }

    void setLoopDurationMS(float dur)
    {
        loopDurationMS = dur;
        loopDuration = loopDurationMS * samplesPerMs;
        if (loopDuration < domain) domain = loopDuration;
    }

    float processEnvelope()
    {
        float interpolatedValue = 0.0f;

        if (incr < domain && domain > 0.0f)
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
            interpolatedValue = (1.0f - distFromPrior) * priorY + distFromPrior * nxtY;
        }

        incr++;
        if (incr >= loopDuration)
            incr = 0;

        return interpolatedValue;
    }

private:
    float samplesPerMs = 44.1f;
    float domainMS, loopDurationMS;

    float incr = 0.0f;
    float domain = 0.0f;
    float loopDuration = 0.0f;

    Array<Point<float>> points;
};
