#ifndef GAME_INTRISTICS_H
#define GAME_INTRISTICS_H
#pragma once

#include "math.h"
#include <xmmintrin.h>

#if COMPILER_MSVC
#define CompletePreviousWritesBeforeFutureWrites _WriteBarrier();

inline u32 AtomicCompareExchangeUInt32(u32 volatile *Value, u32 New, u32 Expected)
{
	u32 Result = _InterlockedCompareExchange((long *)Value, New, Expected);

	return(Result);
}
#else
#endif

//Note: Intel intristics helper defines.
#define OM_MMSquare(A) _mm_mul_ps(A, A)
#define OM_MMIndexF(A, I) ((r32 *)&(A))[I]
#define OM_MMIndexI(A, I) ((u32 *)&(A))[I]

inline s32
RoundReal32ToInt32(r32 Real32)
{
	u32 Result = (u32)_mm_cvtss_si32(_mm_set_ss(Real32));
	return (Result);
}

inline i32
FloorReal32ToInt32(r32 R32)
{
	// TODO: SSE 4.1?
	i32 Result = _mm_cvtss_si32(_mm_floor_ss(_mm_setzero_ps(), _mm_set_ss(R32)));
	return(Result);
}

inline i32
CeilReal32ToInt32(r32 R32)
{
	// TODO: SSE 4.1?
	i32 Result = _mm_cvtss_si32(_mm_ceil_ss(_mm_setzero_ps(), _mm_set_ss(R32)));
	return(Result);
}

inline r32
Sin(r32 Angle)
{
	r32 Result = sinf(Angle);

	return (Result);
}

inline r32
Cos(r32 Angle)
{
	r32 Result = cosf(Angle);

	return (Result);
}

inline r32
ATan2(r32 Y, r32 X)
{
	r32 Result = atan2f(Y, X);

	return (Result);
}

struct bit_scan_result
{
	b32 Found;
	u32 Index;
};

//TODO: Make use of compiler intrinsic
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
SignOf(r32 Value)
{
	r32 Result = (Value >= 0.0f) ? 1.0f : -1.0f;
	
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