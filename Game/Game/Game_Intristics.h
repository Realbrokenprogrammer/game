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

struct bit_scan_result
{
	b32 Found;
	u32 Index;
};

inline bit_scan_result
FindLeastSignificantSetBit(u32 Value)
{
	bit_scan_result Result = {};

	for (u32 Test = 0; Test < 32; ++Test)
	{
		if (Value & (1 << Test))
		{
			Result.Index = Test;
			Result.Found = true;
			break;
		}
	}

	return (Result);
}

inline r32
SquareRoot(r32 Value)
{
	r32 Result = sqrtf(Value);
	return (Result);
}

inline r32
AbsoluteValue(r32 Value)
{
	r32 Result = fabsf(Value);
	return(Result);
}

#endif // GAME_INTRISTICS_H