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