#ifndef GAME_H
#define GAME_H
#pragma once

#include "Platform.h"
#include "Game_Intristics.h"
#include "Game_Math.h"
#include "Game_Memory.h"
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
	AssetState_Loaded,
	AssetState_Locked
};

struct asset_slot
{
	asset_state State;
	loaded_bitmap *Bitmap;
};

struct asset_tag
{
	u32 ID;
	r32 Value;
};

struct asset_bitmap_info
{
	i32 Width;
	i32 Height;

	u32 FirstTagIndex;
	u32 OnePastLastTagIndex;
};

struct asset_group
{
	u32 FirstTagIndex;
	u32 OnePastLastTagIndex;
};

struct game_assets
{
	// TODO: This back-pointer is dumb.
	struct transient_state *TransientState;
	memory_arena Arena;
	debug_platform_read_entire_file *ReadEntireFile;

	asset_slot Bitmaps[GAI_Count];
};

inline loaded_bitmap *
GetBitmap(game_assets *Assets, game_asset_id ID)
{
	loaded_bitmap *Result = Assets->Bitmaps[ID].Bitmap;

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

	r32 Time;
};

struct task_with_memory
{
	b32 BeingUsed;
	memory_arena Arena;

	temporary_memory MemoryFlush;
};

struct transient_state
{
	b32 Initialized;
	memory_arena TransientArena;

	task_with_memory Tasks[4];

	platform_thread_queue *HighPriorityQueue;
	platform_thread_queue *LowPriorityQueue;

	game_assets Assets;
};

om_global_variable platform_add_thread_entry *PlatformAddThreadEntry;
om_global_variable platform_complete_all_work *PlatformCompleteAllThreadWork;
om_internal void LoadAsset(game_assets *Assets, game_asset_id ID);

#endif // GAME_H