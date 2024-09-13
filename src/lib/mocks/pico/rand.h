#pragma once

#include <stdint.h>
#include "../pico.h"


inline uint64_t get_rand_64_value = 0;
inline void get_rand_64_SET(uint64_t v)
{
    get_rand_64_value = v;
}
inline uint64_t get_rand_64()
{
    return get_rand_64_value;
}

