/**
 *	BASIC TYPES
 */
#ifndef OM_NO_DEFINE_TYPES
#include <stdint.h>
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
#endif

#define OM_MIN(a, b) ((a) < (b) ? (a) : (b))
#define OM_MAX(a, b) ((a) > (b) ? (a) : (b))