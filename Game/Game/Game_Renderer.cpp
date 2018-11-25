inline vector4
Unpack4x8Pixel(u32 Pixel)
{
	vector4 Result = { (r32)((Pixel >> 16) & 0xFF),
					   (r32)((Pixel >> 8) & 0xFF),
					   (r32)((Pixel >> 0) & 0xFF),
					   (r32)((Pixel >> 24) & 0xFF) };

	return(Result);
}

inline vector4
SRGB255ToLinear1(vector4 Color)
{
	vector4 Result;

	r32 Inv255 = 1.0f / 255.0f;

	Result.R = Square(Inv255*Color.R);
	Result.G = Square(Inv255*Color.G);
	Result.B = Square(Inv255*Color.B);
	Result.A = Inv255 * Color.A;

	return(Result);
}

inline vector4
Linear1ToSRGB255(vector4 Color)
{
	vector4 Result;

	r32 One255 = 255.0f;

	Result.R = One255 * SquareRoot(Color.R);
	Result.G = One255 * SquareRoot(Color.G);
	Result.B = One255 * SquareRoot(Color.B);
	Result.A = 255.0f*Color.A;

	return(Result);
}

struct bilinear_sample
{
	u32 A, B, C, D;
};
inline bilinear_sample
BilinearSample(loaded_bitmap *Texture, i32 X, i32 Y)
{
	bilinear_sample Result;

	//TODO: Add Pitch to loaded_bitmap.
	u8 *TexelPtr = ((u8 *)Texture->Pixels) + Y * (Texture->Width * 4) + X * sizeof(u32);
	Result.A = *(u32 *)(TexelPtr);
	Result.B = *(u32 *)(TexelPtr + sizeof(u32));
	Result.C = *(u32 *)(TexelPtr + (Texture->Width * 4));
	Result.D = *(u32 *)(TexelPtr + (Texture->Width * 4) + sizeof(u32));

	return(Result);
}

inline vector4
SRGBBilinearBlend(bilinear_sample TexelSample, r32 fX, r32 fY)
{
	vector4 TexelA = Unpack4x8Pixel(TexelSample.A);
	vector4 TexelB = Unpack4x8Pixel(TexelSample.B);
	vector4 TexelC = Unpack4x8Pixel(TexelSample.C);
	vector4 TexelD = Unpack4x8Pixel(TexelSample.D);

	// NOTE: Going from sRGB to "linear" brightness space
	TexelA = SRGB255ToLinear1(TexelA);
	TexelB = SRGB255ToLinear1(TexelB);
	TexelC = SRGB255ToLinear1(TexelC);
	TexelD = SRGB255ToLinear1(TexelD);

	vector4 Result = Lerp(Lerp(TexelA, fX, TexelB),
		fY,
		Lerp(TexelC, fX, TexelD));

	return(Result);
}

om_internal void
DEBUGDrawRect(game_offscreen_buffer *Buffer, vector2 Position, r32 Scale, r32 Rotation, loaded_bitmap *Texture, vector4 Color)
{
	// Degrees to radians.
	Rotation = Rotation * (OM_PI32 / 180.0f);
	
	// Note: Currently not dealing with non-scaled rectangle so we'd probably want to scale the Texture width and height.
	// Scale and rotate Axes based on specified Scale and Rotation.
	vector2 XAxis = (Scale + 1.0f*cosf(Rotation)) * Vector2(cosf(Rotation), sinf(Rotation));
	vector2 YAxis = Perp(XAxis);

	r32 XAxisLength = Length(XAxis);
	r32 YAxisLength = Length(YAxis);

	r32 InvXAxisLengthSq = 1.0f / LengthSquared(XAxis);
	r32 InvYAxisLengthSq = 1.0f / LengthSquared(YAxis);

	//TODO: Use passed color if there is no Texture passed or separate DrawRectangle and DrawBitmap with scaling?
	u32 Color32 =
		((RoundReal32ToInt32(Color.A * 255.0f) << 24) |
		 (RoundReal32ToInt32(Color.R * 255.0f) << 16) |
		 (RoundReal32ToInt32(Color.G * 255.0f) << 8) |
		 (RoundReal32ToInt32(Color.B * 255.0f)));

	int MaxWidth = (Buffer->Width - 1);
	int MaxHeight = (Buffer->Height - 1);

	r32 InvMaxWidth = 1.0f / (r32)MaxWidth;
	r32 InvMaxHeight = 1.0f / (r32)MaxHeight;

	int MinX = MaxWidth;
	int MaxX = 0;
	int MinY = MaxHeight;
	int MaxY = 0;

	vector2 Points[4] = { Position, Position + XAxis, Position + XAxis + YAxis, Position + YAxis };
	for (int PointIndex = 0; PointIndex < OM_ARRAYCOUNT(Points); ++PointIndex)
	{
		vector2 TestPoint = Points[PointIndex];
		int FloorX = FloorReal32ToInt32(TestPoint.x);
		int CeilX = CeilReal32ToInt32(TestPoint.x);
		int FloorY = FloorReal32ToInt32(TestPoint.y);
		int CeilY = CeilReal32ToInt32(TestPoint.y);

		if (MinX > FloorX) { MinX = FloorX; }
		if (MinY > FloorY) { MinY = FloorY; }
		if (MaxX < CeilX)  { MaxX = CeilX; }
		if (MaxY < CeilY)  { MaxY = CeilY; }
	}

	if (MinX < 0) { MinX = 0; }
	if (MinY < 0) { MinY = 0; }
	if (MaxX > MaxWidth)  { MaxX = MaxWidth; }
	if (MaxY > MaxHeight) { MaxY = MaxHeight; }

	u8 *Row = ((u8 *)Buffer->Memory + MinX * Buffer->BytesPerPixel + MinY * Buffer->Pitch);
	for (int Y = MinY; Y <= MaxY; ++Y)
	{
		u32 *Pixel = (u32 *)Row;
		for (int X = MinX; X <= MaxX; ++X)
		{
			vector2 PixelPosition = Vector2Int(X, Y);
			vector2 Distance = PixelPosition - Position;

			r32 Edge0 = Inner(Distance, -Perp(XAxis));
			r32 Edge1 = Inner(Distance - XAxis, -Perp(YAxis));
			r32 Edge2 = Inner(Distance - XAxis - YAxis, Perp(XAxis));
			r32 Edge3 = Inner(Distance - YAxis, Perp(YAxis));

			if ((Edge0 < 0) && (Edge1 < 0) && (Edge2 < 0) && (Edge3 < 0))
			{
				// Texture UV coordinates
				r32 U = InvXAxisLengthSq * Inner(Distance, XAxis);
				r32 V = InvYAxisLengthSq * Inner(Distance, YAxis);

				// TODO: Formalize texture boundaries
				r32 tX = ((U*(r32)(Texture->Width - 2)));
				r32 tY = ((V*(r32)(Texture->Height - 2)));

				i32 X = (i32)tX;
				i32 Y = (i32)tY;

				r32 fX = tX - (r32)X;
				r32 fY = tY - (r32)Y;

				bilinear_sample TexelSample = BilinearSample(Texture, X, Y);
				vector4 Texel = SRGBBilinearBlend(TexelSample, fX, fY);

				Texel = Hadamard(Texel, Color);
				Texel.R = Clamp01(Texel.R);
				Texel.G = Clamp01(Texel.G);
				Texel.B = Clamp01(Texel.B);

				vector4 Dest = { (r32)((*Pixel >> 16) & 0xFF),
								 (r32)((*Pixel >> 8) & 0xFF),
								 (r32)((*Pixel >> 0) & 0xFF),
								 (r32)((*Pixel >> 24) & 0xFF) };

				// NOTE: Going from sRGB to "linear" brightness space
				Dest = SRGB255ToLinear1(Dest);

				vector4 Blended = (1.0f - Texel.A)*Dest + Texel;

				// NOTE: Going from "linear" brightness space to sRGB
				vector4 Blended255 = Linear1ToSRGB255(Blended);

				*Pixel = (((u32)(Blended255.A + 0.5f) << 24) |
						  ((u32)(Blended255.R + 0.5f) << 16) |
						  ((u32)(Blended255.G + 0.5f) << 8) |
						  ((u32)(Blended255.B + 0.5f) << 0));
			}

			++Pixel;
		}

		Row += Buffer->Pitch;
	}
}

om_internal render_blueprint *
CreateRenderBlueprint(render_basis *RenderBasis, u32 MaxPushBufferSize)
{
	render_blueprint *Result = (render_blueprint *)malloc(sizeof(render_blueprint));
	Result->PushBufferBase = (u8 *)malloc(MaxPushBufferSize);

	Result->Basis = RenderBasis;
	Result->PieceCount = 0;

	Result->MaxPushBufferSize = MaxPushBufferSize;
	Result->PushBufferSize = 0;

	return (Result);
}

#define PushRenderBlueprint(Blueprint, Type) (Type*)PushRenderBlueprint_(Blueprint, sizeof(Type), RenderCommand_##Type)
inline render_blueprint_header *
PushRenderBlueprint_(render_blueprint *Blueprint, u32 Size, render_command Type)
{
	render_blueprint_header *Result = 0;

	if ((Blueprint->PushBufferSize + Size) < Blueprint->MaxPushBufferSize)
	{
		Result = (render_blueprint_header *)(Blueprint->PushBufferBase + Blueprint->PushBufferSize);
		Result->Type = Type;
		Blueprint->PushBufferSize += Size;
	}
	else
	{
		InvalidCodePath;
	}

	return (Result);
}

inline void
PushRenderEntry(render_blueprint *Blueprint, loaded_bitmap *Bitmap, vector2 Position, vector2 Offset, vector4 Color)
{
	render_blueprint_bitmap *RenderEntry = (render_blueprint_bitmap *)PushRenderBlueprint(Blueprint, render_blueprint_bitmap);
	if (RenderEntry)
	{
		RenderEntry->Bitmap = Bitmap;
		RenderEntry->Position = Position;
		RenderEntry->Offset = Offset;
		RenderEntry->R = Color.R;
		RenderEntry->G = Color.G;
		RenderEntry->B = Color.B;
		RenderEntry->A = Color.A;
	}
}

inline void
Clear(render_blueprint *Blueprint, vector4 Color)
{
	render_blueprint_clear *RenderEntry = PushRenderBlueprint(Blueprint, render_blueprint_clear);
	if (RenderEntry)
	{
		RenderEntry->Color = Color;
	}
}

inline void
PushLine(render_blueprint *Blueprint, vector2 Start, vector2 End, vector2 Offset, vector4 Color)
{
	render_blueprint_line *RenderEntry = PushRenderBlueprint(Blueprint, render_blueprint_line);
	if (RenderEntry)
	{
		RenderEntry->Start = Start;
		RenderEntry->End = End;
		RenderEntry->Offset = Offset;
		RenderEntry->R = Color.R;
		RenderEntry->G = Color.G;
		RenderEntry->B = Color.B;
		RenderEntry->A = Color.A;
	}
}

inline void
PushCircle()
{
	//TODO: Implement
}

inline void
PushTriangle()
{
	//TODO: Implement
}

inline void
PushRect(render_blueprint *Blueprint, vector2 Position, vector2 Dimension, vector2 Offset, vector4 Color)
{
	render_blueprint_rectangle *RenderEntry = PushRenderBlueprint(Blueprint, render_blueprint_rectangle);
	if (RenderEntry)
	{
		RenderEntry->Position = Position;
		RenderEntry->Dimension = Dimension;
		RenderEntry->Offset = Offset;
		RenderEntry->R = Color.R;
		RenderEntry->G = Color.G;
		RenderEntry->B = Color.B;
		RenderEntry->A = Color.A;
	}
}

// NOTE: Rectangles are drawn from their top left corner not from their center. Might need to change that in the future.
inline void
PushRectOutline(render_blueprint *Blueprint, vector2 Position, vector2 Dimension, vector2 Offset, vector4 Color)
{
	r32 Thickness = 1.0f;

	// NOTE: Top & Bottom.
	PushRect(Blueprint, Position - Vector2(0.0f, 0.0f), Vector2(Dimension.x, Thickness), Offset, Color);
	PushRect(Blueprint, Position + Vector2(0.0f, Dimension.y), Vector2(Dimension.x, Thickness), Offset, Color);

	// NOTE: Left & Right
	PushRect(Blueprint, Position - Vector2(0.0f, 0.0f), Vector2(Thickness, Dimension.y), Offset, Color);
	PushRect(Blueprint, Position + Vector2(Dimension.x, 0.0f), Vector2(Thickness, Dimension.y), Offset, Color);
}

inline void
PushBitmap(render_blueprint *Blueprint, loaded_bitmap *Bitmap, vector2 Position, vector2 Offset, r32 Alpha = 1.0f)
{
	PushRenderEntry(Blueprint, Bitmap, Position, Offset, Vector4(1.0f, 1.0f, 1.0f, Alpha));
}

om_internal void
DestroyRenderBlueprint(render_blueprint *Blueprint)
{
	free(Blueprint->PushBufferBase);
	free(Blueprint);
}