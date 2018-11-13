#ifndef TYPES_H
#define TYPES_H
#pragma once

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

#define OM_PI32 3.14159265359f

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

inline u32
SafeTruncateUInt64(u64 Value)
{
	OM_ASSERT(Value <= 0xFFFFFFFF);
	u32 Result = (u32)Value;
	return (Result);
}

#endif // TYPES_H