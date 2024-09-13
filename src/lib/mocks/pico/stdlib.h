#pragma once

#include <stdint.h>
#include <stdio.h>
#include "../pico.h"

// In pico.h
// inline void panic(const char *fmt, ...)
// {
//     printf("%s", fmt);
// }

// inline void print_binary(unsigned int number)
// {
//     if (number >> 1)
//     {
//         print_binary(number >> 1);
//     }
//     putc((number & 1) ? '1' : '0', stdout);
// }

// template<typename T>
// inline void print_buffer_dec(T *buffer, uint8_t length)
// {
//     printf("Length(%d) ", length);
//     for (uint8_t i = 0; i < length; i++)
//     {
//         printf("%d", buffer[i]);
//         if (i < length-1)
//         {
//             printf(", ");
//         }
//     }
// }

// inline void print_buffer_hex(uint8_t *buffer, uint8_t length)
// {
//     printf("Length(%d) ", length);
//     for (uint8_t i = 0; i < length; i++)
//     {
//         printf("0x%02X", buffer[i]);
//         if (i < length-1)
//         {
//             printf(", ");
//         }
//     }
// }

