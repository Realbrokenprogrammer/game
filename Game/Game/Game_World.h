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

struct world
{
	r32 TileSizeInMeters;
};

#endif // GAME_WORLD_H
