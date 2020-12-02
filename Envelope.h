#ifndef ENVELOPE_H
#define ENVELOPE_H
#include <math.h>
#include <stdlib.h>
#define Q 24
#define ONE floatToFixed(1.0)
#define SATTACK 0
#define SDECAY 1
#define SSUSTAIN 2
#define SRELEASE 3

#include "Arduino.h"
#include "AudioStream.h"
#include "arm_math.h"

inline int32_t floatToFixed(double input)
{
    return (int32_t)(input * (1 << Q));
}

inline int32_t mult32_32_add32(int32_t a, int32_t b, int32_t c)
{
    int32_t t = ((int64_t)a * (int64_t)b) / (1 << Q);
    return t + c;
}
inline int32_t mult32_32(int32_t a, int32_t b)
{
    return ((int64_t)a * (int64_t)b) / (1 << Q);
}

class Envelope : public AudioStream

{
public:
    Envelope() : AudioStream(0, NULL) {}
    void setCoefficients(double _decay, double yf, double _attack, double _sustain, double _release)
    {
        outputMultiplier = floatToFixed(0.8);

        double h = 1.0 / AUDIO_SAMPLE_RATE;
        double midPoint = 0.2;
        double m = _sustain + midPoint * (1 - _sustain);
        double c = (m * m - _sustain) / (2 * m - _sustain - 1);
        double k = (m - c) / (1.0 - c);
        double alpha = 2000.0 * log(k) / _decay;
        multTerm1 = floatToFixed(1.0 + h * alpha);
        constTerm1 = floatToFixed(-h * alpha * c);

        double midPoint2 = midPoint * _sustain;
        m = (yf - midPoint2) / (midPoint2 - 1);
        alpha = 2000.0 / _release * log(m);
        c = 1.0 - (midPoint2 - 1) / (m - 1.0);

        multTerm2 = floatToFixed(1.0 + h * alpha);
        constTerm2 = floatToFixed(-h * alpha * c);
        threshold = floatToFixed(yf);
        sustain = _sustain;

        state = 0;
        lastY = 0;

        attackSlope = (uint32_t)floatToFixed(1000.0 / (_attack * AUDIO_SAMPLE_RATE));
    }

    int32_t nextY()
    {
        switch (state)
        {
        case SATTACK: // attack
            if (lastY >= ONE)
            {
                state = SDECAY;
                return ONE;
            }
            lastY += attackSlope;
            return lastY;
        case SDECAY: // decay
            if (lastY <= sustain)
            {
                state = SSUSTAIN;
                return lastY = sustain;
            }
            return lastY = mult32_32_add32(lastY, multTerm1, constTerm1);
        case SSUSTAIN: // sustain
            return sustain;
        case SRELEASE:
            if (lastY <= threshold)
            {
                return lastY = 0;
            }
            return lastY = mult32_32_add32(lastY, multTerm2, constTerm2);
        }
        return 0;
    }

    void update(void)
    {
        audio_block_t *block = allocate();
        if (block)
        {
            for (int i = 0; i < AUDIO_BLOCK_SAMPLES; ++i)
            {
                block->data[i] = (int16_t)(mult32_32(outputMultiplier, nextY()) / (1 << (Q - 15)));
            }
            transmit(block);
            release(block);
        }
    }

    void noteOn()
    {
        // Sets starting multiplier to 0
        lastY = floatToFixed(0);

        // Sets state to attack
        state = SATTACK;
    }

    void noteOff()
    {
        // Sets state to release
        state = SRELEASE;
    }

private:
    bool shouldDebug = false;
    int32_t sustain;
    int32_t lastY = 0;
    uint32_t attackSlope;
    int32_t multTerm1;
    int32_t multTerm2;
    int32_t constTerm1;
    int32_t constTerm2;
    int32_t threshold;
    int32_t outputMultiplier;
    char state = SATTACK; // 0 = attack, 1 = decay, 2 = sustain, 3 = release
};
#endif