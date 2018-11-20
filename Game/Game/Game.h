#ifndef GAME_H
#define GAME_H
#pragma once

#include "Platform.h"
#include "Game_Intristics.h"
#include "Game_Math.h"
#include "Game_Entity.h"
#include "Game_World.h"
#include "Game_Camera.h"
#include "Game_Physics.h"

//TODO: This could be improved, think of the structure of this later.
enum game_mode
{
	GameMode_None,

	GameMode_Menu,
	GameMode_Cutscene,
	GameMode_World
};

struct game_state
{
	world *World;

	entity *ControlledEntity;

	entity *Player;
	entity *Player2;

	camera Camera;

	int ToneHz;

	loaded_bitmap PlayerBitmap;
	loaded_bitmap GrassBitmap;
	loaded_bitmap WaterBitmap;
	loaded_bitmap SlopeBitmap;
};

#endif // GAME_H