#ifndef TYPES_H
#define TYPES_H
#pragma once

/**
 *	COMPILERS
 *	TODO: Add support for more compilers.
 */
#if !defined(COMPILER_MSVC)
#define COMPILER_MSVC 0
#endif

#if !defined(COMPILER_LLVM)
#define COMPILER_LLVM 0
#endif

#if !COMPILER_MSVC && !COMPILER_LLVM
#if _MSC_VER
#undef COMPILER_MSVC
#define COMPILER_MSVC 1
#else
#undef COMPILER_LLVM
#define COMPILER_LLVM 1
#endif
#endif

#if COMPILER_MSVC
#include <intrin.h>
#elif COMPILER_LLVM
#include <x86intrin.h>
#endif

/**
 *	BASIC TYPES
 */
#ifndef OM_NO_DEFINE_TYPES
#include <stdint.h>
#include <stddef.h>
#include <limits.h>
#include <float.h>
typedef float			r32;
typedef double			r64;
typedef unsigned char	ubyte;
typedef unsigned int	uint;
typedef char			i8;
typedef unsigned char	u8;
typedef int16_t			i16;
typedef uint16_t		u16;
typedef int32_t			i32;
typedef uint32_t		u32;
typedef int64_t			i64;
typedef uint64_t		u64;
typedef i32				b32;

typedef i8 s8;
typedef i16 s16;
typedef i32 s32;
typedef i64 s64;

typedef size_t memory_index;

#define U16MAX 65535
#define I32MIN ((i32)0x80000000)
#define I32MAX ((i32)0x7fffffff)
#define U32MIN 0
#define U32MAX ((u32)-1)
#define U64MAX ((u64)-1)
#define R32MAX FLT_MAX
#define R32MIN -FLT_MAX
#endif

/*
	OM_DEBUG:
		0 - Build for public releasse.
		1 - Build for developer only.
	NOTE: Performance might be used in the future but for now we will stick to Debug.
	OM_PERFORMANCE:
		0 - Slow code welcome. Non performance code.
		1 - No slow code allowed. Performance code.
*/
#if OM_DEBUG
#define OM_ASSERT(Expression) if(!(Expression)) {*(int *)0 = 0;}
#else
#define OM_ASSERT(Expression)
#endif

#define InvalidCodePath OM_ASSERT(!"InvalidCodePath")
#define InvalidDefaultCase default: {InvalidCodePath;} break

#define OM_PI32 3.14159265359f
#define OM_TAU32 6.28318530717958647692f

#define om_kilobytes(Value) ((Value)*1024)
#define om_megabytes(Value) (om_kilobytes(Value)*1024)
#define om_gigabytes(Value) (om_megabytes(Value)*1024)
#define om_terabytes(Value) (om_gigabytes(Value)*1024)

#define om_internal			static
#define om_local_persist	static
#define om_global_variable	static

#define OM_ARRAYCOUNT(Array) (sizeof(Array) / sizeof((Array)[0]))

#define Align8(Value) ((Value + 7) & ~7)

#define OM_MIN(a, b) ((a) < (b) ? (a) : (b))
#define OM_MAX(a, b) ((a) > (b) ? (a) : (b))

#define BITMAP_BYTES_PER_PIXEL 4

// TODO: Change fields to uppercase.
union vector2
{
	struct
	{
		r32 x, y;
	};

	r32 E[2];
};

union vector3
{
	struct
	{
		r32 x, y, z;
	};

	struct
	{
		r32 r, g, b;
	};

	r32 E[3];
};

union vector4
{
	struct
	{
		union
		{
			vector3 XYZ;
			struct
			{
				r32 X, Y, Z;
			};
		};

		r32 W;
	};
	struct
	{
		union
		{
			vector3 RGB;
			struct
			{
				r32 R, G, B;
			};
		};

		r32 A;
	};
	struct
	{
		vector2 xy;
		r32 Ignored0_;
		r32 Ignored1_;
	};
	struct
	{
		r32 Ignored2_;
		vector2 yz;
		r32 Ignored3_;
	};
	struct
	{
		r32 Ignored4_;
		r32 Ignored5_;
		vector2 zw;
	};
	r32 E[4];
};

struct rect2I
{
	i32 MinX;
	i32 MinY;
	i32 MaxX;
	i32 MaxY;
};

struct rect2
{
	vector2 Min;
	vector2 Max;
};

struct rect3
{
	vector3 Min;
	vector3 Max;
};

struct line
{
	vector2 Start;
	vector2 End;
};

struct triangle
{
	vector2 p1;
	vector2 p2;
	vector2 p3;
};

struct circle
{
	r32 Radius;
	vector2 Center;
};

struct transform
{
	vector2 Translation;
	r32 Scale;
	r32 Rotation;
};

inline u32
SafeTruncateUInt64(u64 Value)
{
	OM_ASSERT(Value <= 0xFFFFFFFF);
	u32 Result = (u32)Value;
	return (Result);
}

/*
	OM_ARRAY Implementation.
	Thanks to Sean Barrett.
*/
#include <stdlib.h>

#define OM_ARRAY_RAW(Array) ((int *)(Array) - 2)
#define OM_ARRAY_M(Array) OM_ARRAY_RAW(Array)[0]
#define OM_ARRAY_N(Array) OM_ARRAY_RAW(Array)[1]

#define OM_ARRAY_COUNT(Array) ((Array) ? OM_ARRAY_N(Array) : 0)
#define OM_ARRAY_FREE(Array) ((Array) ? free(OM_ARRAY_RAW(Array)),0 : 0)
#define OM_ARRAY_PUSH(Array, Element) (OM_ARRAY_MAYBEGROW(Array, 1), (Array)[OM_ARRAY_N(Array)++] = (Element))
#define OM_ARRAY_RESERVE(Array, Count) (OM_ARRAY_MAYBEGROW(Array, Count), OM_ARRAY_N(Array)+=(Count), &(Array)[OM_ARRAY_N(Array)-(Count)])
#define OM_ARRAY_LAST(Array) ((Array)[OM_ARRAY_N(Array)-1])

#define OM_ARRAY_NEEDGROW(Array, n) ((Array) == 0 || OM_ARRAY_N(Array)+(n) >= OM_ARRAY_M(Array))
#define OM_ARRAY_MAYBEGROW(Array, n) (OM_ARRAY_NEEDGROW(Array, (n)) ? OM_ARRAY_GROW(Array, n) : 0)
#define OM_ARRAY_GROW(Array, n) (*((void **)&(Array)) = OM__ARRAY_GROWF((Array), (n), sizeof(*(Array))))

//TODO: Variable naming
om_internal void *
OM__ARRAY_GROWF(void *Array, int Increment, int ItemSize)
{
	int cur = Array ? 2 * OM_ARRAY_M(Array) : 0;
	int min = OM_ARRAY_COUNT(Array) + Increment;
	int m = cur > min ? cur : min;
	int *p = (int *) realloc(Array ? OM_ARRAY_RAW(Array) : 0, ItemSize * m + sizeof(int) * 2);
	if (p)
	{
		if (!Array)
		{
			p[1] = 0;
		}
		p[0] = m;
		return p + 2;
	}
	else
	{
		return (void *)(2 * sizeof(int));
	}
}

#endif // TYPES_H