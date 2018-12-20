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

enum game_asset_id
{
	GAI_Grass,
	GAI_Water,
	GAI_SlopeLeft,
	GAI_SlopeRight,
	GAI_Player,

	GAI_Count
};

enum asset_state
{
	AssetState_Unloaded,
	AssetState_Queued,
	AssetState_Loaded
};

struct asset_handle
{
	asset_state State;
	loaded_bitmap *Bitmap;
};

struct game_assets
{
	//TODO: Memory arena for the assets.

	debug_platform_read_entire_file *ReadEntireFile;

	loaded_bitmap *Bitmaps[GAI_Count];

	//TODO: This should later be removed and kept within the memory arena for the assets.
	platform_thread_queue *AssetLoadingQueue;
};

inline loaded_bitmap *
GetBitmap(game_assets *Assets, game_asset_id ID)
{
	loaded_bitmap *Result = Assets->Bitmaps[ID];

	return (Result);
}

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

	game_assets Assets;

	r32 Time;

	platform_thread_queue *RenderQueue;
};

om_global_variable platform_add_thread_entry *PlatformAddThreadEntry;
om_global_variable platform_complete_all_work *PlatformCompleteAllThreadWork;
om_internal void LoadAsset(game_assets *Assets, game_asset_id ID);

#endif // GAME_H