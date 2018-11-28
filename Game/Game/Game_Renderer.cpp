inline u32
Pack4x8Pixel(vector4 Pixel)
{
	u32 Result = ((RoundReal32ToInt32(Pixel.A * 255.0f) << 24) |
				  (RoundReal32ToInt32(Pixel.R * 255.0f) << 16) |
				  (RoundReal32ToInt32(Pixel.G * 255.0f) << 8) |
				  (RoundReal32ToInt32(Pixel.B * 255.0f)));

	return (Result);
}

inline vector4
Unpack4x8Pixel(u32 Pixel)
{
	vector4 Result = { (r32)((Pixel >> 16) & 0xFF),
					   (r32)((Pixel >> 8) & 0xFF),
					   (r32)((Pixel >> 0) & 0xFF),
					   (r32)((Pixel >> 24) & 0xFF) };

	return (Result);
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

	return (Result);
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

	return (Result);
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

	return (Result);
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

	return (Result);
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

	vector2 nXAxis = InvXAxisLengthSq * XAxis;
	vector2 nYAxis = InvYAxisLengthSq * YAxis;

	//TODO: Use passed color if there is no Texture passed or separate DrawRectangle and DrawBitmap with scaling?
	u32 Color32 =
		((RoundReal32ToInt32(Color.A * 255.0f) << 24) |
		 (RoundReal32ToInt32(Color.R * 255.0f) << 16) |
		 (RoundReal32ToInt32(Color.G * 255.0f) << 8) |
		 (RoundReal32ToInt32(Color.B * 255.0f)));

	//TODO: Theese boundaries needs to be properly specified
	int MaxWidth = (Buffer->Width - 1) - 3;
	int MaxHeight = (Buffer->Height - 1) - 3;

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

	//Note: 4 Wide values to use operating through SIMD.
	r32 Inv255 = 1.0f / 255.0f;
	r32 One255 = 255.0f;

	__m128 Inv255_x4 = _mm_set1_ps(Inv255);
	__m128 One255_x4 = _mm_set1_ps(One255);
	__m128 One_x4 = _mm_set1_ps(1.0f);
	__m128 Zero_x4 = _mm_set1_ps(0.0f);
	__m128i Mask255_x4 = _mm_set1_epi32(0xFF);
	__m128 ColorR_x4 = _mm_set1_ps(Color.R);
	__m128 ColorG_x4 = _mm_set1_ps(Color.G);
	__m128 ColorB_x4 = _mm_set1_ps(Color.B);
	__m128 ColorA_x4 = _mm_set1_ps(Color.A);
	__m128 nXAxisX_x4 = _mm_set1_ps(nXAxis.x);
	__m128 nXAxisY_x4 = _mm_set1_ps(nXAxis.y);
	__m128 nYAxisX_x4 = _mm_set1_ps(nYAxis.x);
	__m128 nYAxisY_x4 = _mm_set1_ps(nYAxis.y);
	__m128 PositionX_x4 = _mm_set1_ps(Position.x);
	__m128 PositionY_x4 = _mm_set1_ps(Position.y);

	__m128 WidthSub2 = _mm_set1_ps((r32)(Texture->Width - 2));
	__m128 HeightSub2 = _mm_set1_ps((r32)(Texture->Height - 2));

	u8 *Row = ((u8 *)Buffer->Memory + MinX * Buffer->BytesPerPixel + MinY * Buffer->Pitch);
	for (int Y = MinY; Y <= MaxY; ++Y)
	{
		u32 *Pixel = (u32 *)Row;
		for (int X = MinX; X <= MaxX; X+=4)
		{
			__m128 PixelPositionX = _mm_set_ps((r32)(X + 3), 
											   (r32)(X + 2), 
											   (r32)(X + 1), 
											   (r32)(X + 0));
			__m128 PixelPositionY = _mm_set1_ps((r32)(Y));

			__m128 DX = _mm_sub_ps(PixelPositionX, PositionX_x4);
			__m128 DY = _mm_sub_ps(PixelPositionY, PositionY_x4);
			__m128 U = _mm_add_ps(_mm_mul_ps(DX, nXAxisX_x4), _mm_mul_ps(DY, nXAxisY_x4));
			__m128 V = _mm_add_ps(_mm_mul_ps(DX, nYAxisX_x4), _mm_mul_ps(DY, nYAxisY_x4));

			__m128i WriteMask = _mm_castps_si128(_mm_and_ps(_mm_and_ps(_mm_cmpge_ps(U, Zero_x4),
																	   _mm_cmple_ps(U, One_x4)),
															_mm_and_ps(_mm_cmpge_ps(V, Zero_x4),
																	   _mm_cmple_ps(V, One_x4))));
			{
				__m128i OriginalDestination = _mm_loadu_si128((__m128i *)Pixel);

				U = _mm_min_ps(_mm_max_ps(U, Zero_x4), One_x4);
				V = _mm_min_ps(_mm_max_ps(V, Zero_x4), One_x4);

				__m128 tX = _mm_mul_ps(U, WidthSub2);
				__m128 tY = _mm_mul_ps(V, HeightSub2);

				__m128i FetchX_x4 = _mm_cvttps_epi32(tX);
				__m128i FetchY_x4 = _mm_cvttps_epi32(tY);

				__m128 fX = _mm_sub_ps(tX, _mm_cvtepi32_ps(FetchX_x4));
				__m128 fY = _mm_sub_ps(tY, _mm_cvtepi32_ps(FetchY_x4));

				__m128i SampleA;
				__m128i SampleB;
				__m128i SampleC;
				__m128i SampleD;

				//Note: Load the next four pixels from the texture. Not SIMD friendly so it's done outside SIMD.
				// Equiv to the call to BilinearSample.
				for (int I = 0; I < 4; ++I)
				{
					i32 FetchX = OM_MMIndexI(FetchX_x4, I);
					i32 FetchY = OM_MMIndexI(FetchY_x4, I);

					OM_ASSERT((FetchX >= 0) && (FetchX < Texture->Width));
					OM_ASSERT((FetchY >= 0) && (FetchY < Texture->Height));

					u8 *TexelPtr = ((u8 *)Texture->Pixels) + FetchY * (Texture->Width * 4) + FetchX * sizeof(u32);
					OM_MMIndexI(SampleA, I) = *(u32 *)(TexelPtr);
					OM_MMIndexI(SampleB, I) = *(u32 *)(TexelPtr + sizeof(u32));
					OM_MMIndexI(SampleC, I) = *(u32 *)(TexelPtr + (Texture->Width * 4));
					OM_MMIndexI(SampleD, I) = *(u32 *)(TexelPtr + (Texture->Width * 4) + sizeof(u32));
				}

				//Note: Unpacking Bilinear samples. Equiv to first part of call to SRGBBilinearBlend.
				__m128 TexelAb = _mm_cvtepi32_ps(_mm_and_si128(SampleA, Mask255_x4));
				__m128 TexelAg = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(SampleA, 8),  Mask255_x4));
				__m128 TexelAr = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(SampleA, 16), Mask255_x4));
				__m128 TexelAa = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(SampleA, 24), Mask255_x4));

				__m128 TexelBb = _mm_cvtepi32_ps(_mm_and_si128(SampleB, Mask255_x4));
				__m128 TexelBg = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(SampleB, 8),  Mask255_x4));
				__m128 TexelBr = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(SampleB, 16), Mask255_x4));
				__m128 TexelBa = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(SampleB, 24), Mask255_x4));

				__m128 TexelCb = _mm_cvtepi32_ps(_mm_and_si128(SampleC, Mask255_x4));
				__m128 TexelCg = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(SampleC, 8),  Mask255_x4));
				__m128 TexelCr = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(SampleC, 16), Mask255_x4));
				__m128 TexelCa = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(SampleC, 24), Mask255_x4));

				__m128 TexelDb = _mm_cvtepi32_ps(_mm_and_si128(SampleD, Mask255_x4));
				__m128 TexelDg = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(SampleD, 8),  Mask255_x4));
				__m128 TexelDr = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(SampleD, 16), Mask255_x4));
				__m128 TexelDa = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(SampleD, 24), Mask255_x4));

				//Note: Load the destination colors.
				__m128 Destb = _mm_cvtepi32_ps(_mm_and_si128(OriginalDestination, Mask255_x4));
				__m128 Destg = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(OriginalDestination, 8),  Mask255_x4));
				__m128 Destr = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(OriginalDestination, 16), Mask255_x4));
				__m128 Desta = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(OriginalDestination, 24), Mask255_x4));

				//Note: Going from sRGB to "linear" brightness space. Equiv to call to SRGB255ToLinear1.
				TexelAr = OM_MMSquare(_mm_mul_ps(Inv255_x4, TexelAr));
				TexelAg = OM_MMSquare(_mm_mul_ps(Inv255_x4, TexelAg));
				TexelAb = OM_MMSquare(_mm_mul_ps(Inv255_x4, TexelAb));
				TexelAa = _mm_mul_ps(Inv255_x4, TexelAa);

				TexelBr = OM_MMSquare(_mm_mul_ps(Inv255_x4, TexelBr));
				TexelBg = OM_MMSquare(_mm_mul_ps(Inv255_x4, TexelBg));
				TexelBb = OM_MMSquare(_mm_mul_ps(Inv255_x4, TexelBb));
				TexelBa = _mm_mul_ps(Inv255_x4, TexelBa);

				TexelCr = OM_MMSquare(_mm_mul_ps(Inv255_x4, TexelCr));
				TexelCg = OM_MMSquare(_mm_mul_ps(Inv255_x4, TexelCg));
				TexelCb = OM_MMSquare(_mm_mul_ps(Inv255_x4, TexelCb));
				TexelCa = _mm_mul_ps(Inv255_x4, TexelCa);

				TexelDr = OM_MMSquare(_mm_mul_ps(Inv255_x4, TexelDr));
				TexelDg = OM_MMSquare(_mm_mul_ps(Inv255_x4, TexelDg));
				TexelDb = OM_MMSquare(_mm_mul_ps(Inv255_x4, TexelDb));
				TexelDa = _mm_mul_ps(Inv255_x4, TexelDa);

				//Note: Perform Bilinear blend on texture. 
				__m128 ifX = _mm_sub_ps(One_x4, fX);
				__m128 ifY = _mm_sub_ps(One_x4, fY);

				__m128 l0 = _mm_mul_ps(ifY, ifX);
				__m128 l1 = _mm_mul_ps(ifY, fX);
				__m128 l2 = _mm_mul_ps(fY, ifX);
				__m128 l3 = _mm_mul_ps(fY, fX);

				__m128 Texelr = _mm_add_ps(_mm_add_ps(_mm_mul_ps(l0, TexelAr), _mm_mul_ps(l1, TexelBr)),
										   _mm_add_ps(_mm_mul_ps(l2, TexelCr), _mm_mul_ps(l3, TexelDr)));
				__m128 Texelg = _mm_add_ps(_mm_add_ps(_mm_mul_ps(l0, TexelAg), _mm_mul_ps(l1, TexelBg)),
										   _mm_add_ps(_mm_mul_ps(l2, TexelCg), _mm_mul_ps(l3, TexelDg)));
				__m128 Texelb = _mm_add_ps(_mm_add_ps(_mm_mul_ps(l0, TexelAb), _mm_mul_ps(l1, TexelBb)),
										   _mm_add_ps(_mm_mul_ps(l2, TexelCb), _mm_mul_ps(l3, TexelDb)));
				__m128 Texela = _mm_add_ps(_mm_add_ps(_mm_mul_ps(l0, TexelAa), _mm_mul_ps(l1, TexelBa)),
										   _mm_add_ps(_mm_mul_ps(l2, TexelCa), _mm_mul_ps(l3, TexelDa)));

				//Note: Modulate inoming color
				Texelr = _mm_mul_ps(Texelr, ColorR_x4);
				Texelg = _mm_mul_ps(Texelg, ColorG_x4);
				Texelb = _mm_mul_ps(Texelb, ColorB_x4);
				Texela = _mm_mul_ps(Texela, ColorA_x4);

				//Note: Clamp RGB values
				Texelr = _mm_min_ps(_mm_max_ps(Texelr, Zero_x4), One_x4);
				Texelg = _mm_min_ps(_mm_max_ps(Texelg, Zero_x4), One_x4);
				Texelb = _mm_min_ps(_mm_max_ps(Texelb, Zero_x4), One_x4);

				//Note: Going from sRGB to "linear" brightness space. Equiv to call to SRGB255ToLinear1.
				Destr = OM_MMSquare(_mm_mul_ps(Inv255_x4, Destr));
				Destg = OM_MMSquare(_mm_mul_ps(Inv255_x4, Destg));
				Destb = OM_MMSquare(_mm_mul_ps(Inv255_x4, Destb));
				Desta = _mm_mul_ps(Inv255_x4, Desta);

				//Note: Performing blend on destination
				__m128 InvTexelA = _mm_sub_ps(One_x4, Texela);
				__m128 Blendedr = _mm_add_ps(_mm_mul_ps(InvTexelA, Destr), Texelr);
				__m128 Blendedg = _mm_add_ps(_mm_mul_ps(InvTexelA, Destg), Texelg);
				__m128 Blendedb = _mm_add_ps(_mm_mul_ps(InvTexelA, Destb), Texelb);
				__m128 Blendeda = _mm_add_ps(_mm_mul_ps(InvTexelA, Desta), Texela);

				//Note: Going from "linear" brightness space to sRGB
				Blendedr = _mm_mul_ps(One255_x4, _mm_sqrt_ps(Blendedr));
				Blendedg = _mm_mul_ps(One255_x4, _mm_sqrt_ps(Blendedg));
				Blendedb = _mm_mul_ps(One255_x4, _mm_sqrt_ps(Blendedb));
				Blendeda = _mm_mul_ps(One255_x4, Blendeda);

				//Note: Conversion to integer values.
				__m128i Intr = _mm_cvtps_epi32(Blendedr);
				__m128i Intg = _mm_cvtps_epi32(Blendedg);
				__m128i Intb = _mm_cvtps_epi32(Blendedb);
				__m128i Inta = _mm_cvtps_epi32(Blendeda);

				//Note: Shifts into right slots.
				__m128i Sr = _mm_slli_epi32(Intr, 16);
				__m128i Sg = _mm_slli_epi32(Intg, 8);
				__m128i Sb = Intb;
				__m128i Sa = _mm_slli_epi32(Inta, 24);

				__m128i Out = _mm_or_si128(_mm_or_si128(Sr, Sg), _mm_or_si128(Sb, Sa));

				__m128i MaskedOut = _mm_or_si128(_mm_and_si128(WriteMask, Out),
												 _mm_andnot_si128(WriteMask, OriginalDestination));

				_mm_storeu_si128((__m128i *)Pixel, MaskedOut);
			}

			Pixel += 4;
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