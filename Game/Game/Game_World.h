#ifndef GAME_WORLD_H
#define GAME_WORLD_H
#pragma once

//TODO: Don't use this standard library laters
#include <vector>
#include <map>
#include <algorithm>

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

enum world_tile_type
{
	TileType_None,

	TileType_Temp,
	TileType_Grass
};

struct world_tile
{
	u32 Value;
	b32 Collideable;
};

#define MAX_LAYERS 8
#define MAX_ENTITIES 10000
struct world_layer
{
	u32 EntityCount;
	entity Entities[MAX_ENTITIES];
};

struct world
{
	/*
		Layers:
		1 - Foreground General 2
			Tiles can't be placed here. Objects can't kill you. This is most for foreground looks / paralax.
		2 - Foreground Tile
			Tile layer that goes over the main ground layer. Might be used to hide things. Player won't interract with this.
		3 - Foreground Tile General
			Can't place tiles here but you can place objects.
		4 - Active Tile
			Everything is collideable here. Player will interract with objects and tiles here.
		5 - Back Tile General
			Mostly for objects. Things you want to place behind walls like traps etc.
		6 - Back Tile
			Background tiles.
		7 - Back General 2
			For paralax.
		8 - Back General 3
			For background.
		
		Props:
			* Rocks

		Animated props:
			* Rain
			* Clouds

		Objects: - Usually things you interract with or kill you.
			* End level object. (Object you touch to "win")
			* Traps, saws, lava.
			NOTE: Should be able to set a start and finish destination to make the object move between those positions.

	*/

	u32 WorldWidth;
	u32 WorldHeight;

	//TODO: Should we group this into a struct?
	u32 CellSize;
	u32 Columns;
	u32 Rows;
	std::vector<std::vector<entity *>> Buckets; // Use array instead

	world_layer Layers[8];
};

#endif // GAME_WORLD_H
