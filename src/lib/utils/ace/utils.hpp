#pragma once

#include "hardware/pio.h"
#include "stdint.h"
#include "stdio.h"


// Find a free pio and state machine and load the program into it.
// Returns false if this fails
bool add_pio_program(const pio_program_t *program, PIO *pio_hw, uint *sm, uint *offset);

/**
 * SHow a spinner on the terminal to indicate OpenACE is working on something
*/
void showSpinner();
/**
 * Remove the spinner
*/
void clearSpinner();

void manchechesterEncode(uint8_t *destination, const uint8_t* source, uint8_t sourceLength);

/**
 * Manchester Decode a buffer of maximum length 255
*/
void manchesterDecode(uint8_t *buffer, uint8_t bufferLength);
/**
 * Decode a manchester encoded buffer into a destination buffer of half the length of the source
*/
void manchesterDecode(uint8_t *destination, const uint8_t *source, uint8_t sourceLength);

/**
 * Decode a manchester encoded buffer into a destination buffer of half the length of the source
 * THis verison includes the err frame for galagan correction
*/
void manchesterDecode(uint8_t *destination, uint8_t *err, const uint8_t *source, uint8_t sourceLength);


/**
 * Calculate the parity of a given byte buffer
*/
uint8_t buffersParity8(const uint8_t *buffer, uint16_t bytes);

/**
 * Swap to low order and high order 8 bit
*/
inline uint16_t swapBytes16(uint16_t value)
{
    return (value >> 8) | (value << 8);
}

