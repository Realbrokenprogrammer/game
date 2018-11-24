#ifndef GAME_RENDERER_H
#define GAME_RENDERER_H
#pragma once

struct render_basis
{
	vector2 Position;
};

// NOTE: Discriminated union.
enum render_command
{
	RenderCommand_render_blueprint_clear,
	RenderCommand_render_blueprint_line,
	RenderCommand_render_blueprint_circle,
	RenderCommand_render_blueprint_triangle,
	RenderCommand_render_blueprint_rectangle,
	RenderCommand_render_blueprint_bitmap
};

struct render_blueprint_header
{
	render_command Type;
};

struct render_blueprint_clear
{
	render_blueprint_header Header;
	vector4 Color;
};

struct render_blueprint_line
{
	render_blueprint_header Header;
	vector2 Start;
	vector2 End;
	vector2 Offset;
	r32 R, G, B, A;
};

struct render_blueprint_circle
{
	render_blueprint_header Header;
	vector2 Position;
	vector2 Offset;
	r32 R, G, B, A;
	r32 Radius;
};

struct render_blueprint_triangle
{
	render_blueprint_header Header;
	vector2 Position;
	vector2 Offset;
	r32 R, G, B, A;
	vector2 Point1;
	vector2 Point2;
	vector2 Point3;
};

struct render_blueprint_rectangle
{
	render_blueprint_header Header;
	vector2 Position;
	vector2 Dimension; //TODO: Should this be Min / Max instead?
	vector2 Offset;
	r32 R, G, B, A;
};

struct render_blueprint_bitmap
{
	render_blueprint_header Header;
	loaded_bitmap *Bitmap;
	vector2 Position;
	vector2 Offset;
	r32 R, G, B, A;
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