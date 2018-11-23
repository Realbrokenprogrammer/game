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

inline vector4
Vector4(r32 X, r32 Y, r32 Z, r32 W)
{
	vector4 Result;

	Result.X = X;
	Result.Y = Y;
	Result.Z = Z;
	Result.W = W;

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
	vector2 Result = A * B;

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

inline r32
Lerp(r32 A, r32 B, r32 t)
{
	r32 Result = A + (t * (B - A));

	return (Result);
}

inline vector2
Lerp(vector2 A, vector2 B, r32 t)
{
	vector2 Result = A + (t * (B - A));

	return (Result);
}

inline vector2
Perp(vector2 A)
{
	vector2 Result = { A.y, -A.x };
	
	return (Result);
}

inline r32
Square(r32 Value)
{
	r32 Result = Value * Value;

	return (Result);
}

inline r32
Inner(vector2 A, vector2 B)
{
	r32 Result = A.x * B.x + A.y * B.y;

	return (Result);
}

inline vector3
Cross(vector3 A, vector3 B)
{
	vector3 Result;
	
	Result.x = A.y * B.z - A.z * B.y;
	Result.y = A.z * B.x - A.x * B.z;
	Result.z = A.x * B.y - A.y * B.x;

	return (Result);
}

inline r32
LengthSquared(vector2 A)
{
	r32 Result = Inner(A, A);

	return (Result);
}

inline r32
Length(vector2 A)
{
	r32 Result = SquareRoot(LengthSquared(A));
	return (Result);
}

//TODO: Check that this is correct.
inline vector2
Normalize(vector2 A)
{
	vector2 Result = {};
	
	if (Length(A) > 0)
	{
		Result.x = A.x / Length(A);
		Result.y = A.y / Length(A);
	}

	return (Result);
}

inline vector2
GetDimension(rect2 Rect)
{
	vector2 Result = (Rect.Max - Rect.Min);
	return (Result);
}

inline vector2
GetCenter(rect2 Rect)
{
	vector2 Result = 0.5f*(Rect.Min + Rect.Max);
	return (Result);
}

inline void
GetPoints(rect2 Rect, vector2 *Points)
{
	OM_ASSERT(OM_ARRAYCOUNT(Points) == 4);

	Points[0] = Rect.Min;
	Points[1] = { Rect.Min.x, Rect.Max.y };
	Points[2] = { Rect.Min.y, Rect.Max.x };
	Points[3] = Rect.Max;
}

inline r32
Clamp(r32 Min, r32 Value, r32 Max)
{
	r32 Result = Value;

	if (Result < Min)
	{
		Result = Min;
	}
	else if (Result > Max)
	{
		Result = Max;
	}

	return (Result);
}

inline vector2
Clamp(vector2 Min, vector2 Value, vector2 Max)
{
	vector2 Result;

	Result.x = Clamp(Min.x, Value.x, Max.x);
	Result.y = Clamp(Min.y, Value.y, Max.y);

	return (Result);
}

#endif // GAME_MATH_H
