#pragma once

#include <stdint.h>

typedef unsigned int uint;




// mock panic
inline void panic_unsupported(const char *file, int line, const char *func, const char *msg)
{
    printf("panic_unsupported: %s:%d %s: %s\n", file, line, func, msg);
    exit(1);
}


inline void panic(const char *msg)
{
    printf("%s\n", msg);
    exit(1);
}