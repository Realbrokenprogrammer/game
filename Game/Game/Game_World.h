#ifndef GAME_WORLD_H
#define GAME_WORLD_H
#pragma once

struct world_difference
{
	vector2 deltaXY;
};

struct world_position
{
	i32 X;
	i32 Y;
	i32 Z;
};

struct world_tile2D
{
	i32 X;
	i32 Y;
	i32 Z;
};

struct world
{
	r32 TileSizeInMeters;
	r32 PixelsPerTileInMeters;

	world_tile2D Tiles[4096];

};

#endif // GAME_WORLD_H
