#include "Arduino.h"
#ifndef INSTRUMENTINTERFACE_H
#define INSTRUMENTINTERFACE_H

class Value
{
public:
    Value(float _m, float _M, float initialValue, const char *_nameTag, int steps = 40, bool _isExp = false)
    {
        isExp = _isExp;
        if (isExp)
        {
            float lambda = -log(_M / _m) / ((float)steps);
            delta = 1 - lambda;
        }
        else
        {
            delta = (_M - _m) / ((float)steps);
        }
        _min = _m;
        _max = _M;
        value = initialValue;
        nameTag = _nameTag;
    }
    float value;
    const char *nameTag;

    void increment()
    {
        if (isExp)
        {
            value *= delta;
        }
        else
        {
            value += delta;
        }
        value = min(value, _max);
    }

    void decrement()
    {
        if (isExp)
        {
            value /= delta;
        }
        else
        {
            value -= delta;
        }
        value = max(value, _min);
    }

    // private:
    float delta;
    float _min;
    float _max;
    bool isExp;
};
#endif