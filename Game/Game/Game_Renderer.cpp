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

om_internal void *
PushRenderElement(render_blueprint *Blueprint, u32 Size)
{
	void *Result = 0;

	if ((Blueprint->PushBufferSize + Size) < Blueprint->MaxPushBufferSize)
	{
		Result = (Blueprint->PushBufferBase + Blueprint->PushBufferSize);
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
	render_entry_bitmap *RenderEntry = (render_entry_bitmap *)PushRenderElement(Blueprint, sizeof(render_entry_bitmap));
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
PushBitmap(render_blueprint *Blueprint, loaded_bitmap *Bitmap, vector2 Position, vector2 Offset, r32 Alpha = 1.0f)
{
	PushRenderEntry(Blueprint, Bitmap, Position, Offset, Vector4(1.0f, 1.0f, 1.0f, Alpha));
}

// NOTE: This should be free'd every update and then we create a new render_blueprint for the
// new frame.
om_internal void
DestroyRenderBlueprint(render_blueprint *Blueprint)
{
	free(Blueprint->PushBufferBase);
	free(Blueprint);
}