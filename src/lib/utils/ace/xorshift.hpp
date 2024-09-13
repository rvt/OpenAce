#pragma once

#include <stdint.h>
#include "ace/coreutils.hpp"

/**
 * Simple random generator using XorShift.
 * Note: Class is not thread safe
 * https://en.wikipedia.org/wiki/Xorshift
*/
class XorShift
{
private:
    uint32_t state;
    // Constructor
    XorShift() : state(CoreUtils::msSinceBoot()) {}
    XorShift(uint32_t state_) : state(state_) {}
public:
    uint32_t rand()
    {
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;
        return state;
    }
};

