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

struct bilinear_sample
{
	u32 A, B, C, D;
};
inline bilinear_sample
BilinearSample(loaded_bitmap *Texture, i32 X, i32 Y)
{
	bilinear_sample Result;

	u8 *TexelPtr = ((u8 *)Texture->Pixels) + Y * Texture->Pitch + X * sizeof(u32);
	Result.A = *(u32 *)(TexelPtr);
	Result.B = *(u32 *)(TexelPtr + sizeof(u32));
	Result.C = *(u32 *)(TexelPtr + Texture->Pitch);
	Result.D = *(u32 *)(TexelPtr + Texture->Pitch + sizeof(u32));

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

//#include "iacaMarks.h"

om_internal void
DEBUGDrawTransformedBitmap(game_offscreen_buffer *Buffer, vector2 Position, r32 Scale, r32 Rotation, 
	loaded_bitmap *Texture, vector4 Color, rect2I ClipRect, b32 Even)
{
	//BEGIN_TIMED_MILLISECOND_BLOCK(DEBUGDrawTransformedBitmap);

	// Degrees to radians.
	Rotation = Rotation * (OM_PI32 / 180.0f);

	// Premultiply alpha
	Color.RGB *= Color.A;

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

	rect2I FillRect = InvertedInfinityRectangle();

	vector2 Points[4] = { Position, Position + XAxis, Position + XAxis + YAxis, Position + YAxis };
	for (int PointIndex = 0; PointIndex < OM_ARRAYCOUNT(Points); ++PointIndex)
	{
		vector2 TestPoint = Points[PointIndex];
		int FloorX = FloorReal32ToInt32(TestPoint.x);
		int CeilX = CeilReal32ToInt32(TestPoint.x);
		int FloorY = FloorReal32ToInt32(TestPoint.y);
		int CeilY = CeilReal32ToInt32(TestPoint.y);

		if (FillRect.MinX > FloorX) { FillRect.MinX = FloorX; }
		if (FillRect.MinY > FloorY) { FillRect.MinY = FloorY; }
		if (FillRect.MaxX < CeilX)  { FillRect.MaxX = CeilX; }
		if (FillRect.MaxY < CeilY)  { FillRect.MaxY = CeilY; }
	}

	FillRect = Intersection(ClipRect, FillRect);
	if (!Even == (FillRect.MinY & 1))
	{
		FillRect.MinY += 1;
	}

	if (HasArea(FillRect))
	{
		int FillWidth = FillRect.MaxX - FillRect.MinX;
		int FillWidthAlign = FillWidth & 3;
		if (FillWidthAlign > 0)
		{
			int Adjustment = (3 - FillWidthAlign);

			//TODO: Change into something sane
			switch (Adjustment)
			{
				case 1: {} break;
				case 2: {} break;
				case 3: {} break;
			}
			FillWidth += Adjustment;
			FillRect.MinX = FillRect.MaxX - FillWidth;
		}

		int MinY = FillRect.MinY;
		int MaxY = FillRect.MaxY;
		int MinX = FillRect.MinX;
		int MaxX = FillRect.MaxX;

		if (MinX < 0) { MinX = 0; }
		if (MinY < 0) { MinY = 0; }
		if (MaxX > MaxWidth) { MaxX = MaxWidth; }
		if (MaxY > MaxHeight) { MaxY = MaxHeight; }

		u8 *Row = ((u8 *)Buffer->Memory + MinX * Buffer->BytesPerPixel + MinY * Buffer->Pitch);
		i32 RowAdvance = 2 * Buffer->Pitch;

		for (int Y = MinY; Y <= MaxY; Y+=2)
		{
			u32 *Pixel = (u32 *)Row;
			for (int X = MinX; X <= MaxX; ++X)
			{
				//IACA_START;

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

				//IACA_END;
			}

			Row += RowAdvance;
		}
	}
	//END_TIMED_MILLISECOND_BLOCK(DEBUGDrawTransformedBitmap);
}

om_internal void
SoftwareDrawTransformedBitmap(game_offscreen_buffer *Buffer, vector2 Position, r32 Scale, r32 Rotation, 
	loaded_bitmap *Texture, vector4 Color, rect2I ClipRect, b32 Even)
{
	//BEGIN_TIMED_MILLISECOND_BLOCK(SoftwareDrawTransformedBitmap);

	// Degrees to radians.
	Rotation = Rotation * (OM_PI32 / 180.0f);

	// Premultiply alpha
	Color.RGB *= Color.A;

	// Note: Currently not dealing with non-scaled rectangle so we'd probably want to scale the Texture width and height.
	// Scale and rotate Axes based on specified Scale and Rotation.
	vector2 XAxis = (Scale + 1.0f*cosf(Rotation)) * Vector2(cosf(Rotation), sinf(Rotation));
	vector2 YAxis = Perp(XAxis);

	r32 XAxisLength = Length(XAxis);
	r32 YAxisLength = Length(YAxis);

	vector2 NXAxis = (YAxisLength / XAxisLength) * XAxis;
	vector2 NYAxis = (XAxisLength / YAxisLength) * YAxis;

	r32 InvXAxisLengthSq = 1.0f / LengthSquared(XAxis);
	r32 InvYAxisLengthSq = 1.0f / LengthSquared(YAxis);

	//TODO: Use passed color if there is no Texture passed or separate DrawRectangle and DrawBitmap with scaling?
	u32 Color32 =
		((RoundReal32ToInt32(Color.A * 255.0f) << 24) |
		 (RoundReal32ToInt32(Color.R * 255.0f) << 16) |
		 (RoundReal32ToInt32(Color.G * 255.0f) << 8) |
		 (RoundReal32ToInt32(Color.B * 255.0f)));

	rect2I FillRect = InvertedInfinityRectangle();

	vector2 Points[4] = { Position, Position + XAxis, Position + XAxis + YAxis, Position + YAxis };
	for (int PointIndex = 0; PointIndex < OM_ARRAYCOUNT(Points); ++PointIndex)
	{
		vector2 TestPoint = Points[PointIndex];
		int FloorX = FloorReal32ToInt32(TestPoint.x);
		int CeilX = CeilReal32ToInt32(TestPoint.x) + 1;
		int FloorY = FloorReal32ToInt32(TestPoint.y);
		int CeilY = CeilReal32ToInt32(TestPoint.y) + 1;

		if (FillRect.MinX > FloorX) { FillRect.MinX = FloorX; }
		if (FillRect.MinY > FloorY) { FillRect.MinY = FloorY; }
		if (FillRect.MaxX < CeilX)  { FillRect.MaxX = CeilX; }
		if (FillRect.MaxY < CeilY)  { FillRect.MaxY = CeilY; }
	}

	FillRect = Intersection(ClipRect, FillRect);
	if (!Even == (FillRect.MinY & 1))
	{
		FillRect.MinY += 1;
	}

	if (HasArea(FillRect))
	{
		__m128i StartupClipMask = _mm_set1_epi8(-1);
		int FillWidth = FillRect.MaxX - FillRect.MinX;
		int FillWidthAlign = FillWidth & 3;
		if (FillWidthAlign > 0)
		{
			int Adjustment = (4 - FillWidthAlign);

			//TODO: Change this into something sane.
			switch (Adjustment)
			{
				case 1: {StartupClipMask = _mm_slli_si128(StartupClipMask, 1 * 4); } break;
				case 2: {StartupClipMask = _mm_slli_si128(StartupClipMask, 2 * 4); } break;
				case 3: {StartupClipMask = _mm_slli_si128(StartupClipMask, 3 * 4); } break;
			}
			FillWidth += Adjustment;
			FillRect.MinX = FillRect.MaxX - FillWidth;
		}

		vector2 nXAxis = InvXAxisLengthSq * XAxis;
		vector2 nYAxis = InvYAxisLengthSq * YAxis;

		//Note: 4 Wide values to use operating through SIMD.
		r32 Inv255 = 1.0f / 255.0f;
		r32 One255 = 255.0f;

		__m128 Inv255_x4 = _mm_set1_ps(Inv255);
		__m128 One255_x4 = _mm_set1_ps(One255);
		__m128 One_x4 = _mm_set1_ps(1.0f);
		__m128 Four_x4 = _mm_set1_ps(4.0f);
		__m128 Zero_x4 = _mm_set1_ps(0.0f);
		__m128i Mask255_x4 = _mm_set1_epi32(0xFF);
		__m128i MaskFFFF = _mm_set1_epi32(0xFFFF);
		__m128i MaskFF00FF = _mm_set1_epi32(0x00FF00FF);
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
		__m128 MaxColorValue = _mm_set1_ps(255.0f*255.0f);
		__m128i TexturePitch_x4 = _mm_set1_epi32(Texture->Pitch);

		__m128 WidthSub2 = _mm_set1_ps((r32)(Texture->Width - 2));
		__m128 HeightSub2 = _mm_set1_ps((r32)(Texture->Height - 2));

		u8 *Row = ((u8 *)Buffer->Memory + FillRect.MinX * Buffer->BytesPerPixel + FillRect.MinY * Buffer->Pitch);
		i32 RowAdvance = 2 * Buffer->Pitch;

		void *TextureMemory = Texture->Pixels;
		i32 TexturePitch = Texture->Pitch;

		int MinY = FillRect.MinY;
		int MaxY = FillRect.MaxY;
		int MinX = FillRect.MinX;
		int MaxX = FillRect.MaxX;
		for (int Y = MinY; Y < MaxY; Y += 2)
		{
			__m128 PixelPositionY = _mm_set1_ps((r32)Y);
			PixelPositionY = _mm_sub_ps(PixelPositionY, PositionY_x4);
			__m128 PynX = _mm_mul_ps(PixelPositionY, nXAxisY_x4);
			__m128 PynY = _mm_mul_ps(PixelPositionY, nYAxisY_x4);

			__m128 PixelPositionX = _mm_set_ps((r32)(MinX + 3),
											   (r32)(MinX + 2),
											   (r32)(MinX + 1),
											   (r32)(MinX + 0));
			PixelPositionX = _mm_sub_ps(PixelPositionX, PositionX_x4);

			__m128i ClipMask = StartupClipMask;

			u32 *Pixel = (u32 *)Row;
			for (int X = MinX; X < MaxX; X += 4)
			{
				//IACA_START;

				__m128 U = _mm_add_ps(_mm_mul_ps(PixelPositionX, nXAxisX_x4), PynX);
				__m128 V = _mm_add_ps(_mm_mul_ps(PixelPositionX, nYAxisX_x4), PynY);

				__m128i WriteMask = _mm_castps_si128(_mm_and_ps(_mm_and_ps(_mm_cmpge_ps(U, Zero_x4),
																		   _mm_cmple_ps(U, One_x4)),
																_mm_and_ps(_mm_cmpge_ps(V, Zero_x4),
																		   _mm_cmple_ps(V, One_x4))));

				WriteMask = _mm_and_si128(WriteMask, ClipMask);

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

					FetchX_x4 = _mm_slli_epi32(FetchX_x4, 2);
					FetchY_x4 = _mm_or_si128(_mm_mullo_epi16(FetchY_x4, TexturePitch_x4),
											 _mm_slli_epi32(_mm_mulhi_epi16(FetchY_x4, TexturePitch_x4), 16));
					__m128i Fetch_x4 = _mm_add_epi32(FetchX_x4, FetchY_x4);

					i32 Fetch0 = OM_MMIndexI(Fetch_x4, 0);
					i32 Fetch1 = OM_MMIndexI(Fetch_x4, 1);
					i32 Fetch2 = OM_MMIndexI(Fetch_x4, 2);
					i32 Fetch3 = OM_MMIndexI(Fetch_x4, 3);

					u8 *TexelPtr0 = ((u8 *)TextureMemory) + Fetch0;
					u8 *TexelPtr1 = ((u8 *)TextureMemory) + Fetch1;
					u8 *TexelPtr2 = ((u8 *)TextureMemory) + Fetch2;
					u8 *TexelPtr3 = ((u8 *)TextureMemory) + Fetch3;

					__m128i SampleA = _mm_setr_epi32(*(u32 *)(TexelPtr0),
													 *(u32 *)(TexelPtr1),
													 *(u32 *)(TexelPtr2),
													 *(u32 *)(TexelPtr3));

					__m128i SampleB = _mm_setr_epi32(*(u32 *)(TexelPtr0 + sizeof(u32)),
													 *(u32 *)(TexelPtr1 + sizeof(u32)),
													 *(u32 *)(TexelPtr2 + sizeof(u32)),
													 *(u32 *)(TexelPtr3 + sizeof(u32)));

					__m128i SampleC = _mm_setr_epi32(*(u32 *)(TexelPtr0 + TexturePitch),
													 *(u32 *)(TexelPtr1 + TexturePitch),
													 *(u32 *)(TexelPtr2 + TexturePitch),
													 *(u32 *)(TexelPtr3 + TexturePitch));

					__m128i SampleD = _mm_setr_epi32(*(u32 *)(TexelPtr0 + TexturePitch + sizeof(u32)),
													 *(u32 *)(TexelPtr1 + TexturePitch + sizeof(u32)),
													 *(u32 *)(TexelPtr2 + TexturePitch + sizeof(u32)),
													 *(u32 *)(TexelPtr3 + TexturePitch + sizeof(u32)));

					// Note: Unpacking bilinear samples
					__m128i TexelArb = _mm_and_si128(SampleA, MaskFF00FF);
					__m128i TexelAag = _mm_and_si128(_mm_srli_epi32(SampleA, 8), MaskFF00FF);
					TexelArb = _mm_mullo_epi16(TexelArb, TexelArb);
					__m128 TexelAa = _mm_cvtepi32_ps(_mm_srli_epi32(TexelAag, 16));
					TexelAag = _mm_mullo_epi16(TexelAag, TexelAag);

					__m128i TexelBrb = _mm_and_si128(SampleB, MaskFF00FF);
					__m128i TexelBag = _mm_and_si128(_mm_srli_epi32(SampleB, 8), MaskFF00FF);
					TexelBrb = _mm_mullo_epi16(TexelBrb, TexelBrb);
					__m128 TexelBa = _mm_cvtepi32_ps(_mm_srli_epi32(TexelBag, 16));
					TexelBag = _mm_mullo_epi16(TexelBag, TexelBag);

					__m128i TexelCrb = _mm_and_si128(SampleC, MaskFF00FF);
					__m128i TexelCag = _mm_and_si128(_mm_srli_epi32(SampleC, 8), MaskFF00FF);
					TexelCrb = _mm_mullo_epi16(TexelCrb, TexelCrb);
					__m128 TexelCa = _mm_cvtepi32_ps(_mm_srli_epi32(TexelCag, 16));
					TexelCag = _mm_mullo_epi16(TexelCag, TexelCag);

					__m128i TexelDrb = _mm_and_si128(SampleD, MaskFF00FF);
					__m128i TexelDag = _mm_and_si128(_mm_srli_epi32(SampleD, 8), MaskFF00FF);
					TexelDrb = _mm_mullo_epi16(TexelDrb, TexelDrb);
					__m128 TexelDa = _mm_cvtepi32_ps(_mm_srli_epi32(TexelDag, 16));
					TexelDag = _mm_mullo_epi16(TexelDag, TexelDag);

					//Note: Load the destination colors.
					__m128 Destb = _mm_cvtepi32_ps(_mm_and_si128(OriginalDestination, Mask255_x4));
					__m128 Destg = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(OriginalDestination, 8), Mask255_x4));
					__m128 Destr = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(OriginalDestination, 16), Mask255_x4));
					__m128 Desta = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(OriginalDestination, 24), Mask255_x4));

					//Note: Going from sRGB to "linear" brightness space. Equiv to call to SRGB255ToLinear1.
					__m128 TexelAr = _mm_cvtepi32_ps(_mm_srli_epi32(TexelArb, 16));
					__m128 TexelAg = _mm_cvtepi32_ps(_mm_and_si128(TexelAag, MaskFFFF));
					__m128 TexelAb = _mm_cvtepi32_ps(_mm_and_si128(TexelArb, MaskFFFF));
					
					__m128 TexelBr = _mm_cvtepi32_ps(_mm_srli_epi32(TexelBrb, 16));
					__m128 TexelBg = _mm_cvtepi32_ps(_mm_and_si128(TexelBag, MaskFFFF));
					__m128 TexelBb = _mm_cvtepi32_ps(_mm_and_si128(TexelBrb, MaskFFFF));
					
					__m128 TexelCr = _mm_cvtepi32_ps(_mm_srli_epi32(TexelCrb, 16));
					__m128 TexelCg = _mm_cvtepi32_ps(_mm_and_si128(TexelCag, MaskFFFF));
					__m128 TexelCb = _mm_cvtepi32_ps(_mm_and_si128(TexelCrb, MaskFFFF));
					
					__m128 TexelDr = _mm_cvtepi32_ps(_mm_srli_epi32(TexelDrb, 16));
					__m128 TexelDg = _mm_cvtepi32_ps(_mm_and_si128(TexelDag, MaskFFFF));
					__m128 TexelDb = _mm_cvtepi32_ps(_mm_and_si128(TexelDrb, MaskFFFF));

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
					Texelr = _mm_min_ps(_mm_max_ps(Texelr, Zero_x4), MaxColorValue);
					Texelg = _mm_min_ps(_mm_max_ps(Texelg, Zero_x4), MaxColorValue);
					Texelb = _mm_min_ps(_mm_max_ps(Texelb, Zero_x4), MaxColorValue);

					//Note: Going from sRGB to "linear" brightness space. Equiv to call to SRGB255ToLinear1.
					Destr = OM_MMSquare(Destr);
					Destg = OM_MMSquare(Destg);
					Destb = OM_MMSquare(Destb);
					
					//Note: Performing blend on destination
					__m128 InvTexelA = _mm_sub_ps(One_x4, _mm_mul_ps(Inv255_x4, Texela));
					__m128 Blendedr = _mm_add_ps(_mm_mul_ps(InvTexelA, Destr), Texelr);
					__m128 Blendedg = _mm_add_ps(_mm_mul_ps(InvTexelA, Destg), Texelg);
					__m128 Blendedb = _mm_add_ps(_mm_mul_ps(InvTexelA, Destb), Texelb);
					__m128 Blendeda = _mm_add_ps(_mm_mul_ps(InvTexelA, Desta), Texela);

					//Note: Going from "linear" brightness space to sRGB
					Blendedr = _mm_mul_ps(Blendedr, _mm_rsqrt_ps(Blendedr));
					Blendedg = _mm_mul_ps(Blendedg, _mm_rsqrt_ps(Blendedg));
					Blendedb = _mm_mul_ps(Blendedb, _mm_rsqrt_ps(Blendedb));
					Blendeda = Blendeda;

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

				PixelPositionX = _mm_add_ps(PixelPositionX, Four_x4);
				Pixel += 4;
				ClipMask = _mm_set1_epi8(-1);

				//IACA_END;
			}

			Row += RowAdvance;
		}
	}

	//END_TIMED_MILLISECOND_BLOCK(SoftwareDrawTransformedBitmap);
}

om_internal void
SoftwareDrawRect(game_offscreen_buffer *Buffer, vector2 Min, vector2 Max, r32 R, r32 G, r32 B)
{
	i32 MinX = RoundReal32ToInt32(Min.x);
	i32 MinY = RoundReal32ToInt32(Min.y);
	i32 MaxX = RoundReal32ToInt32(Max.x);
	i32 MaxY = RoundReal32ToInt32(Max.y);

	if (MinX < 0)
	{
		MinX = 0;
	}

	if (MinY < 0)
	{
		MinY = 0;
	}

	if (MaxX > Buffer->Width)
	{
		MaxX = Buffer->Width;
	}

	if (MaxY > Buffer->Height)
	{
		MaxY = Buffer->Height;
	}

	u32 Color =
		((RoundReal32ToInt32(R * 255.0f) << 16) |
		(RoundReal32ToInt32(G * 255.0f) << 8) |
			(RoundReal32ToInt32(B * 255.0f)));

	u8 *Row = ((u8 *)Buffer->Memory + MinX * Buffer->BytesPerPixel + MinY * Buffer->Pitch);
	for (int Y = MinY; Y < MaxY; ++Y)
	{
		u32 *Pixel = (u32 *)Row;
		for (int X = MinX; X < MaxX; ++X)
		{
			*Pixel++ = Color;
		}

		Row += Buffer->Pitch;
	}
}

om_internal void
SoftwareDrawCircle(game_offscreen_buffer *Buffer, vector2 Center, r32 Radius, r32 R, r32 G, r32 B)
{
	u32 Color =
		((RoundReal32ToInt32(R * 255.0f) << 16) |
		(RoundReal32ToInt32(G * 255.0f) << 8) |
			(RoundReal32ToInt32(B * 255.0f)));

	for (r32 Angle = 0.0f; Angle < 360.0f; Angle++)
	{
		i32 X = (i32)(Center.x - Radius * cosf(Angle));
		i32 Y = (i32)(Center.y - Radius * sinf(Angle));

		if (X < 0)
		{
			X = 0;
		}

		if (Y < 0)
		{
			Y = 0;
		}

		if (Y > Buffer->Height)
		{
			Y = Buffer->Height;
		}

		if (X > Buffer->Width)
		{
			X = Buffer->Width;
		}

		u32 *Pixel = ((u32 *)Buffer->Memory + Buffer->Width * Y + X);
		*Pixel = Color;
	}
}

om_internal void
SoftwareDrawLine(game_offscreen_buffer *Buffer, vector2 Start, vector2 End, r32 R, r32 G, r32 B)
{
	if (Start.x > End.x)
	{
		vector2 Temp = Start;
		Start = End;
		End = Temp;
	}

	if (Start.x < 0)
	{
		Start.x = 0;
	}

	if (Start.y < 0)
	{
		Start.y = 0;
	}

	if (End.x > Buffer->Width)
	{
		End.x = (r32)Buffer->Width;
	}

	if (End.y > Buffer->Height)
	{
		End.y = (r32)Buffer->Height;
	}

	r32 Coefficient = 0.0f;
	if (End.x - Start.x == 0)
	{
		Coefficient = 10000;
		if (End.y - Start.y < 0)
		{
			Coefficient = Coefficient * -1;
		}
	}
	else
	{
		Coefficient = (End.y - Start.y) / (End.x - Start.x);
	}

	r32 CX;
	r32 CY;

	if (Coefficient < 0)
	{
		CX = 1 / (-1 + Coefficient) * -1;
		CY = Coefficient / (-1 + Coefficient) * -1;
	}
	else
	{
		CX = 1 / (1 + Coefficient);
		CY = Coefficient / (1 + Coefficient);
	}

	CX *= 1.00001f;
	CY *= 1.00001f;

	r32 ToLength = SquareRoot((End.x - Start.x) * (End.x - Start.x) + (End.y - Start.y) * (End.y - Start.y));
	r32 Length = 0;
	r32 Increment = SquareRoot(CX * CX + CY * CY);

	u32 Color =
		((RoundReal32ToInt32(R * 255.0f) << 16) |
		(RoundReal32ToInt32(G * 255.0f) << 8) |
			(RoundReal32ToInt32(B * 255.0f)));

	for (int Index = 0; Length < ToLength + 0.5f; ++Index)
	{
		r32 X = Start.x + CX * Index;
		r32 Y = Start.y + CY * Index;

		u32 *Pixel = ((u32 *)Buffer->Memory + Buffer->Width * (u32)Y + (u32)X);
		*Pixel = Color;

		Length += Increment;
	}
}

om_internal void
SoftwareDrawTriangle(game_offscreen_buffer *Buffer, triangle Triangle, r32 R, r32 G, r32 B)
{
	SoftwareDrawLine(Buffer, Triangle.p1, Triangle.p2, R, G, B);
	SoftwareDrawLine(Buffer, Triangle.p2, Triangle.p3, R, G, B);
	SoftwareDrawLine(Buffer, Triangle.p3, Triangle.p1, R, G, B);
}

om_internal void
SoftwareDrawBitmap(game_offscreen_buffer *Buffer, loaded_bitmap *Bitmap, vector2 Target, r32 ColorAlpha)
{
	i32 MinX = RoundReal32ToInt32(Target.x);
	i32 MinY = RoundReal32ToInt32(Target.y);
	i32 MaxX = MinX + Bitmap->Width;
	i32 MaxY = MinY + Bitmap->Height;

	i32 SourceOffsetX = 0;
	if (MinX < 0)
	{
		SourceOffsetX = -MinX;
		MinX = 0;
	}

	i32 SourceOffsetY = 0;
	if (MinY < 0)
	{
		SourceOffsetY = -MinY;
		MinY = 0;
	}

	if (MaxX > Buffer->Width)
	{
		MaxX = Buffer->Width;
	}

	if (MaxY > Buffer->Height)
	{
		MaxY = Buffer->Height;
	}

	u32 *SourceRow = (Bitmap->Pixels + SourceOffsetX) + (SourceOffsetY * Bitmap->Width);
	u8 *DestinationRow = ((u8 *)Buffer->Memory + MinX * Buffer->BytesPerPixel + MinY * Buffer->Pitch);
	for (int Y = MinY; Y < MaxY; ++Y)
	{
		u32 *Destination = (u32 *)DestinationRow;
		u32 *Source = SourceRow;
		for (int X = MinX; X < MaxX; ++X)
		{
			*Destination++ = *Source++;
		}

		DestinationRow += Buffer->Pitch;
		SourceRow += Bitmap->Width;
	}
}

om_internal render_blueprint *
CreateRenderBlueprint(game_assets *Assets, render_basis *RenderBasis, u32 MaxPushBufferSize)
{
	//TODO: This malloc shouldn't be here?!?
	render_blueprint *Result = (render_blueprint *)malloc(sizeof(render_blueprint));
	Result->PushBufferBase = (u8 *)malloc(MaxPushBufferSize);

	Result->Basis = RenderBasis;

	Result->MaxPushBufferSize = MaxPushBufferSize;
	Result->PushBufferSize = 0;

	Result->Assets = Assets;

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
PushCircle(render_blueprint *Blueprint, vector2 Position, r32 Radius, vector2 Offset, vector4 Color)
{
	render_blueprint_circle *RenderEntry = PushRenderBlueprint(Blueprint, render_blueprint_circle);
	if (RenderEntry)
	{
		RenderEntry->Position = Position;
		RenderEntry->Offset = Offset;
		RenderEntry->Radius = Radius;
		RenderEntry->R = Color.R;
		RenderEntry->G = Color.G;
		RenderEntry->B = Color.B;
		RenderEntry->A = Color.A;
	}
}

inline void
PushTriangle(render_blueprint *Blueprint, vector2 Position, vector2 Point1, vector2 Point2, vector2 Point3, vector2 Offset, vector4 Color)
{
	render_blueprint_triangle *RenderEntry = PushRenderBlueprint(Blueprint, render_blueprint_triangle);
	if (RenderEntry)
	{
		RenderEntry->Position = Position;
		RenderEntry->Offset = Offset;
		RenderEntry->Point1 = Point1;
		RenderEntry->Point2 = Point2;
		RenderEntry->Point3 = Point3;
		RenderEntry->R = Color.R;
		RenderEntry->G = Color.G;
		RenderEntry->B = Color.B;
		RenderEntry->A = Color.A;
	}
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

//TODO: Pass in actual transform instead of position, scale and rotation separately.
inline void
PushBitmap(render_blueprint *Blueprint, loaded_bitmap *Bitmap, vector2 Position, r32 Scale, r32 Rotation, vector2 Offset, vector4 Color)
{
	render_blueprint_bitmap *RenderEntry = (render_blueprint_bitmap *)PushRenderBlueprint(Blueprint, render_blueprint_bitmap);
	if (RenderEntry)
	{
		RenderEntry->Bitmap = Bitmap;
		RenderEntry->Position = Position;
		RenderEntry->Scale = Scale;
		RenderEntry->Rotation = Rotation;
		RenderEntry->Offset = Offset;
		RenderEntry->R = Color.R;
		RenderEntry->G = Color.G;
		RenderEntry->B = Color.B;
		RenderEntry->A = Color.A;
	}
}

inline void
PushBitmap(render_blueprint *Blueprint, bitmap_id ID, vector2 Position, r32 Scale, r32 Rotation, vector2 Offset, vector4 Color)
{
	loaded_bitmap *Bitmap = GetBitmap(Blueprint->Assets, ID);
	if (Bitmap)
	{
		PushBitmap(Blueprint, Bitmap, Position, Scale, Rotation, Offset, Color);
	}
	else
	{
		LoadBitmap(Blueprint->Assets, ID);
	}
}

//TODO: Naming
//TODO: Add clipping rectangle to rest of the drawing functions.
om_internal void
RenderToBuffer(render_blueprint *RenderBlueprint, game_offscreen_buffer *Buffer, rect2I ClipRect, b32 Even)
{
	//BEGIN_TIMED_BLOCK(RenderToBuffer);

	for (u32 BaseAddress = 0; BaseAddress < RenderBlueprint->PushBufferSize;)
	{
		render_blueprint_header *Header = (render_blueprint_header *)(RenderBlueprint->PushBufferBase + BaseAddress);

		switch (Header->Type)
		{
		case RenderCommand_render_blueprint_clear:
		{
			render_blueprint_clear *Body = (render_blueprint_clear *)Header;

			SoftwareDrawRect(Buffer, Vector2(0.0f, 0.0f), Vector2((r32)Buffer->Width, (r32)Buffer->Height), Body->Color.R, Body->Color.G, Body->Color.B);

			BaseAddress += sizeof(*Body);
		} break;
		case RenderCommand_render_blueprint_line:
		{
			render_blueprint_line *Body = (render_blueprint_line *)Header;
			//TODO: Needs a position vector to be drawn relative too. See Triangle.
			vector2 StartPosition = Body->Start - RenderBlueprint->Basis->Position;
			vector2 EndPosition = Body->End - RenderBlueprint->Basis->Position;

			SoftwareDrawLine(Buffer, StartPosition, EndPosition, Body->R, Body->G, Body->B);

			BaseAddress += sizeof(*Body);
		} break;
		case RenderCommand_render_blueprint_circle:
		{
			render_blueprint_circle *Body = (render_blueprint_circle *)Header;
			vector2 Position = Body->Position - RenderBlueprint->Basis->Position;

			SoftwareDrawCircle(Buffer, Position, Body->Radius, Body->R, Body->G, Body->B);

			BaseAddress += sizeof(*Body);
		} break;
		case RenderCommand_render_blueprint_triangle:
		{
			render_blueprint_triangle *Body = (render_blueprint_triangle *)Header;
			vector2 Position = Body->Position - RenderBlueprint->Basis->Position;
			vector2 Point1 = Body->Point1 + Position;
			vector2 Point2 = Body->Point2 + Position;
			vector2 Point3 = Body->Point3 + Position;

			//TODO: Do we want SoftwareDrawTriangle to take a triangle struct?
			triangle T = { Point1, Point2, Point3 };

			SoftwareDrawTriangle(Buffer, T, Body->R, Body->G, Body->B);

			BaseAddress += sizeof(*Body);
		} break;
		case RenderCommand_render_blueprint_rectangle:
		{
			render_blueprint_rectangle *Body = (render_blueprint_rectangle *)Header;
			vector2 Position = Body->Position - RenderBlueprint->Basis->Position;
			SoftwareDrawRect(Buffer, Position, Position + Body->Dimension, Body->R, Body->G, Body->B);
			BaseAddress += sizeof(*Body);
		} break;
		case RenderCommand_render_blueprint_bitmap:
		{
			render_blueprint_bitmap *Body = (render_blueprint_bitmap *)Header;
			vector2 Position = Body->Position - RenderBlueprint->Basis->Position;

			//NOTE: THIS IS TO ENABLE / DISABLE THE UNOPTIMIZED OR THE OPTIMIZED RENDERER.
#if 1
			//DrawBitmap(Buffer, Body->Bitmap, Position, Body->A);
			DEBUGDrawTransformedBitmap(Buffer, Position, Body->Scale, Body->Rotation, Body->Bitmap,
				Vector4(Body->R, Body->G, Body->B, Body->A), ClipRect, Even);
#else
			SoftwareDrawTransformedBitmap(Buffer, Position, Body->Scale, Body->Rotation, Body->Bitmap, 
				Vector4(Body->R, Body->G, Body->B, Body->A), ClipRect, Even);
#endif
			BaseAddress += sizeof(*Body);
		} break;

		InvalidDefaultCase;
		}
	}

	//END_TIMED_BLOCK(RenderToBuffer);
}

struct partitioned_render_work
{
	render_blueprint *RenderBlueprint;
	game_offscreen_buffer *Buffer;
	rect2I ClipRect;
};

om_internal
PLATFORM_THREAD_QUEUE_CALLBACK(DoPartitionedRenderWork)
{
	partitioned_render_work *Work = (partitioned_render_work *)Data;

	RenderToBuffer(Work->RenderBlueprint, Work->Buffer, Work->ClipRect, true);
	RenderToBuffer(Work->RenderBlueprint, Work->Buffer, Work->ClipRect, false);
}

//TODO: Pass bitmap instead of entire offscreen buffer?
// Note: Splits up the rendering into a 4x4 grid. This is to allow multiple threads
// render different parts of the grid.
om_internal void
PerformPartitionedRendering(platform_thread_queue *RenderQueue, render_blueprint *RenderBlueprint, game_offscreen_buffer *Buffer)
{
	BEGIN_TIMED_MILLISECOND_BLOCK(Rendering);
	int const PartitionCountX = 4;
	int const PartitionCountY = 4;
	partitioned_render_work WorkArray[PartitionCountX * PartitionCountY];

	int PartitionWidth = Buffer->Width / PartitionCountX;
	int PartitionHeight = Buffer->Height / PartitionCountY;
	
	int WorkCount = 0;
	for (int PartitionY = 0; PartitionY < PartitionCountY; ++PartitionY)
	{
		for (int PartitionX = 0; PartitionX < PartitionCountX; ++PartitionX)
		{
			partitioned_render_work *Work = WorkArray + WorkCount++;

			//TODO: Buffers can overflow. Needs fixing.
			rect2I ClipRect;

			ClipRect.MinX = PartitionX * PartitionWidth + 4;
			ClipRect.MaxX = ClipRect.MinX + PartitionWidth - 4;
			ClipRect.MinY = PartitionY * PartitionHeight + 4;
			ClipRect.MaxY = ClipRect.MinY + PartitionHeight - 4;

			Work->RenderBlueprint = RenderBlueprint;
			Work->Buffer = Buffer;
			Work->ClipRect = ClipRect;

			Platform.AddThreadEntry(RenderQueue, DoPartitionedRenderWork, Work);
		}
	}

	Platform.CompleteAllThreadWork(RenderQueue);
	END_TIMED_MILLISECOND_BLOCK(Rendering);
}

om_internal void
DestroyRenderBlueprint(render_blueprint *Blueprint)
{
	free(Blueprint->PushBufferBase);
	free(Blueprint);
}