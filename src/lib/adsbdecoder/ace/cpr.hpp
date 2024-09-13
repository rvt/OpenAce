
#pragma once

#include <stdint.h>

void decodeCPR(bool fflag, uint32_t even_cprlat, uint32_t even_cprlon, uint32_t odd_cprlat, uint32_t odd_cprlon, float *pfLat, float *pfLon);
