#ifndef GAME_RENDERER_H
#define GAME_RENDERER_H
#pragma once

struct render_basis
{
	vector2 Position;
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