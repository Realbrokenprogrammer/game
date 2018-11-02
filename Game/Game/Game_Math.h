#ifndef GAME_MATH_H
#define GAME_MATH_H
#pragma once

inline vector2
Vector2Int(i32 X, i32 Y)
{
	vector2 Result = { (r32)X, (r32)Y };

	return (Result);
}

inline vector2
Vector2Int(u32 X, u32 Y)
{
	vector2 Result = { (r32)X, (r32)Y };

	return (Result);
}

inline vector2
Vector2(r32 X, r32 Y)
{
	vector2 Result;

	Result.x = X;
	Result.y = Y;

	return (Result);
}

inline vector3
Vector3(r32 X, r32 Y, r32 Z)
{
	vector3 Result;

	Result.x = X;
	Result.y = Y;
	Result.z = Z;

	return (Result);
}

inline r32
Square(r32 Value)
{
	r32 Result = Value * Value;

	return (Result);
}

inline vector2
operator *(r32 A, vector2 B)
{
	vector2 Result;

	Result.x = A * B.x;
	Result.y = A * B.y;

	return (Result);
}

inline vector2
operator *(vector2 B, r32 A)
{
	vector2 Result = A*B;

	return (Result);
}

inline vector2
operator *=(vector2 &B, r32 A)
{
	B = A * B;

	return (B);
}

inline vector2
operator -(vector2 A)
{
	vector2 Result;

	Result.x = -A.x;
	Result.y = -A.y;

	return (Result);
}

inline vector2
operator +(vector2 A, vector2 B)
{
	vector2 Result;

	Result.x = A.x + B.x;
	Result.y = A.y + B.y;

	return (Result);
}

inline vector2 &
operator +=(vector2 &A, vector2 B)
{
	A = A + B;

	return(A);
}

inline vector2
operator -(vector2 A, vector2 B)
{
	vector2 Result;

	Result.x = A.x - B.x;
	Result.y = A.y - B.y;

	return(Result);
}

inline vector2 &
operator -=(vector2 &A, vector2 B)
{
	A = A - B;

	return (A);
}

#endif // GAME_MATH_H
