#ifndef _UTIL_CPP_
#define _UTIL_CPP_

#include "math.h"
#include "resources_structures.hpp"
#include <random>

inline float nextTime(float rateParameter)
{
    srand(5);
    return -logf(1.0f - (float) rand() / (RAND_MAX)) / rateParameter;
}

#endif
