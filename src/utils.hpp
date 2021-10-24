#ifndef __UTILS_HPP__
#define __UTILS_HPP__

#include <stdint.h>
#include <Particle.h>
#define N_TO_B_ENDIAN_2(x)  ((((uint16_t)(x) & 0x00FF) << 8) | (((uint16_t)(x) & 0xFF00) >> 8))
#define N_TO_B_ENDIAN_4(x)  ((((uint32_t)(x) & 0x000000FF) << 24) | (((uint32_t)(x) & 0x0000FF00) << 8) | (((uint32_t)(x) & 0x00FF0000) >> 8) | (((uint32_t)(x) & 0xFF000000) >> 24))

#define B_TO_N_ENDIAN_2(x)  N_TO_B_ENDIAN_2(x)
#define B_TO_N_ENDIAN_4(x)  N_TO_B_ENDIAN_4(x)

void UTIL_sleepUntil(system_tick_t ticks);

#endif