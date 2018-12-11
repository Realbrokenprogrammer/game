#ifndef GAME_H
#define GAME_H
#pragma once

#include "Platform.h"
#include "Game_Intristics.h"
#include "Game_Math.h"
#include "Game_Physics.h"
#include "Game_Entity.h"
#include "Game_World.h"
#include "Game_Camera.h"
#include "Game_Renderer.h"

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
	loaded_bitmap SlopeBitmapLeft;
	loaded_bitmap SlopeBitmapRight;

	r32 Time;

	platform_thread_queue *RenderQueue;
};

om_global_variable platform_add_thread_entry *PlatformAddThreadEntry;
om_global_variable platform_complete_all_work *PlatformCompleteAllThreadWork;

#endif // GAME_H