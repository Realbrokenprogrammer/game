#ifndef GAME_INTRISTICS_H
#define GAME_INTRISTICS_H
#pragma once

#include "math.h"
#include <xmmintrin.h>

inline s32
RoundReal32ToInt32(r32 Real32)
{
	u32 Result = (u32)_mm_cvtss_si32(_mm_set_ss(Real32));
	return (Result);
}

#endif // GAME_INTRISTICS_H