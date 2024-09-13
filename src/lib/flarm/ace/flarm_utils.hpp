#pragma once

#include <stdint.h>

// unpack integer value from a pseudo-floating format
template <uint8_t mbits, uint8_t ebits, bool sbits>
int descale(unsigned int value)
{
    unsigned int offset = (1 << mbits);
    unsigned int signbit = (offset << ebits);
    unsigned int negative = 0;
    if (sbits)
    {
        negative = (value & signbit);
    }
    value &= (signbit - 1); // ignores signbit and higher
    if (value >= offset)
    {
        unsigned int exp = value >> mbits;
        value &= (offset - 1);
        value += offset;
        value <<= exp;
        value -= offset;
    }
    return (negative ? -(int)value : value);
}

template <uint8_t mbits, uint8_t ebits, uint8_t sbits>
unsigned int enscale(int value)
{
    unsigned int offset = (1 << mbits);
    unsigned int signbit = (offset << ebits);
    unsigned int negative = 0;
    if (value < 0)
    {
        if (!sbits)
        {
            // underflow
            return 0; // clamp to minimum
        }
        value = -value;
        negative = signbit;
    }
    if (static_cast<unsigned int>(value) < offset)
    {
        return (negative | static_cast<unsigned int>(value)); // just for efficiency
    }
    unsigned int exp = 0;
    unsigned int mantissa = offset + static_cast<unsigned int>(value);
    unsigned int mlimit = offset + offset - 1;
    unsigned int elimit = signbit - 1;
    while (mantissa > mlimit)
    {
        mantissa >>= 1;
        exp += offset;
        if (exp > elimit)
        {
            // overflow
            return (negative | elimit); // clamp to maximum
        }
    }
    mantissa -= offset;
    return (negative | exp | mantissa);
}

uint16_t lonDivisor(float latitude);

uint16_t flarmCalculateChecksum(const uint8_t* flarm_pkt, uint8_t length);
