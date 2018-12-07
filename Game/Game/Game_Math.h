#ifndef GAME_MATH_H
#define GAME_MATH_H
#pragma once

//TODO: Group vector2, vector3 and vector4 operations
//TODO: Match vector4 Lerp parameters with my own implemented Lerp

// Rect2I Operations
inline rect2I 
Intersection(rect2I A, rect2I B)
{
	rect2I Result;

	Result.MinX = (A.MinX < B.MinX) ? B.MinX : A.MinX;
	Result.MinY = (A.MinY < B.MinY) ? B.MinY : A.MinY;
	Result.MaxX = (A.MaxX > B.MaxX) ? B.MaxX : A.MaxX;
	Result.MaxY = (A.MaxY > B.MaxY) ? B.MaxY : A.MaxY;

	return (Result);
}

inline rect2I
Union(rect2I A, rect2I B)
{
	rect2I Result;

	Result.MinX = (A.MinX < B.MinX) ? A.MinX : B.MinX;
	Result.MinY = (A.MinY < B.MinY) ? A.MinY : B.MinY;
	Result.MaxX = (A.MaxX > B.MaxX) ? A.MaxX : B.MaxX;
	Result.MaxY = (A.MaxY > B.MaxY) ? A.MaxY : B.MaxY;

	return (Result);
}

inline b32
HasArea(rect2I A)
{
	b32 Result = ((A.MinX < A.MaxX) && (A.MinY < A.MaxY));

	return (Result);
}

inline rect2I
InvertedInfinityRectangle(void)
{
	rect2I Result;
	
	Result.MinX = Result.MinY = INT_MAX;
	Result.MaxX = Result.MaxY = -INT_MAX;
	
	return (Result);
}

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
	vector2 Result = { -A.y, A.x };
	
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

inline r32
Clamp01(r32 Value)
{
	r32 Result = Clamp(0.0f, Value, 1.0f);
	
	return (Result);
}

// Vector3 Operations
inline vector3
operator*(r32 A, vector3 B)
{
	vector3 Result;

	Result.x = A * B.x;
	Result.y = A * B.y;
	Result.z = A * B.z;

	return(Result);
}

inline vector3
operator*(vector3 B, r32 A)
{
	vector3 Result = A * B;

	return(Result);
}

inline vector3 &
operator*=(vector3 &B, r32 A)
{
	B = A * B;

	return(B);
}

// Vector4 Operations
inline vector4
operator*(r32 A, vector4 B)
{
	vector4 Result;

	Result.X = A * B.X;
	Result.Y = A * B.Y;
	Result.Z = A * B.Z;
	Result.W = A * B.W;

	return(Result);
}

inline vector4
operator*(vector4 B, r32 A)
{
	vector4 Result = A * B;

	return(Result);
}

inline vector4 &
operator*=(vector4 &B, r32 A)
{
	B = A * B;

	return(B);
}

inline vector4
operator-(vector4 A)
{
	vector4 Result;

	Result.X = -A.X;
	Result.Y = -A.Y;
	Result.Z = -A.Z;
	Result.W = -A.W;

	return(Result);
}

inline vector4
operator+(vector4 A, vector4 B)
{
	vector4 Result;

	Result.X = A.X + B.X;
	Result.Y = A.Y + B.Y;
	Result.Z = A.Z + B.Z;
	Result.W = A.W + B.W;

	return(Result);
}

inline vector4 &
operator+=(vector4 &A, vector4 B)
{
	A = A + B;

	return(A);
}

inline vector4
operator-(vector4 A, vector4 B)
{
	vector4 Result;

	Result.X = A.X - B.X;
	Result.Y = A.Y - B.Y;
	Result.Z = A.Z - B.Z;
	Result.W = A.W - B.W;

	return(Result);
}

inline vector4 &
operator-=(vector4 &A, vector4 B)
{
	A = A - B;

	return(A);
}

inline vector4
Hadamard(vector4 A, vector4 B)
{
	vector4 Result = { A.X*B.X, A.Y*B.Y, A.Z*B.Z, A.W*B.W };

	return(Result);
}

inline r32
Inner(vector4 A, vector4 B)
{
	r32 Result = A.X*B.X + A.Y*B.Y + A.Z*B.Z + A.W*B.W;

	return(Result);
}

inline r32
LengthSq(vector4 A)
{
	r32 Result = Inner(A, A);

	return(Result);
}

inline r32
Length(vector4 A)
{
	r32 Result = SquareRoot(LengthSq(A));
	return(Result);
}

inline vector4
Clamp01(vector4 Value)
{
	vector4 Result;

	Result.X = Clamp01(Value.X);
	Result.Y = Clamp01(Value.Y);
	Result.Z = Clamp01(Value.Z);
	Result.W = Clamp01(Value.W);

	return(Result);
}

inline vector4
Lerp(vector4 A, r32 t, vector4 B)
{
	vector4 Result = (1.0f - t)*A + t * B;

	return(Result);
}

#endif // GAME_MATH_H
