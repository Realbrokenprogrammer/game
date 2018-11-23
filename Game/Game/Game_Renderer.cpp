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

// NOTE: This should be free'd every update and then we create a new render_blueprint for the
// new frame.
om_internal void
DestroyRenderBlueprint(render_blueprint *Blueprint)
{
	free(Blueprint->PushBufferBase);
	free(Blueprint);
}