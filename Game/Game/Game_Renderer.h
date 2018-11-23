#ifndef GAME_RENDERER_H
#define GAME_RENDERER_H
#pragma once

struct render_basis
{
	vector2 Position;
};

struct render_entry_bitmap
{
	loaded_bitmap *Bitmap;
	vector2 Position;
	vector2 Offset;
	r32 R, G, B, A;
};

enum render_command
{
	RenderCommand_Line,
	RenderCommand_Circle,
	RenderCommand_Triangle,
	RenderCommand_Rectangle,
	RenderCommand_RectangleOutline,
	RenderCommand_Bitmap
};

struct render_blueprint
{
	render_basis *Basis;
	u32 PieceCount;
	
	u32 MaxPushBufferSize;
	u32 PushBufferSize;
	u8 *PushBufferBase;
};

#endif // GAME_RENDERER_H