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


//TODO: Memory arena allignment offset
//TODO: Memory arena size remaining
//TODO: Memory arena push helper functions
//TODO: Memory arena Subarena helper function
//TODO: ZeroStruct / Size helper function.
//TODO: Move to its own .h and .cpp?
//TODO: Once all above is resolved: Review new asset loading code.
struct memory_arena
{
	memory_index Size;
	u8 *Base;
	memory_index Used;

	i32 TempCount;
};

struct temporary_memory
{
	memory_arena *Arena;
	memory_index Used;
};

inline void
InitializeMemoryArena(memory_arena *Arena, memory_index Size, void *Base)
{
	Arena->Size = Size;
	Arena->Base = (u8 *)Base;
	Arena->Used = 0;
	Arena->TempCount = 0;
}

inline temporary_memory
CreateTemporaryMemory(memory_arena *Arena)
{
	temporary_memory Result;

	Result.Arena = Arena;
	Result.Used = Arena->Used;

	++Arena->TempCount;

	return (Result);
}

inline void
DestroyTemporaryMemory(temporary_memory TemporaryMemory)
{
	memory_arena *Arena = TemporaryMemory.Arena;
	OM_ASSERT(Arena->Used >= TemporaryMemory.Used);

	Arena->Used = TemporaryMemory.Used;
	OM_ASSERT(Arena->TempCount > 0);

	--Arena->TempCount;
}

inline void
CheckMemoryArena(memory_arena *Arena)
{
	OM_ASSERT(Arena->TempCount == 0);
}

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
	//TODO: Memory arena for the assets.

	debug_platform_read_entire_file *ReadEntireFile;

	asset_slot Bitmaps[GAI_Count];

	//TODO: This should later be removed and kept within the memory arena for the assets.
	platform_thread_queue *AssetLoadingQueue;
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

	int ToneHz;

	game_assets Assets;

	r32 Time;

	platform_thread_queue *RenderQueue;
};

om_global_variable platform_add_thread_entry *PlatformAddThreadEntry;
om_global_variable platform_complete_all_work *PlatformCompleteAllThreadWork;
om_internal void LoadAsset(game_assets *Assets, game_asset_id ID);

#endif // GAME_H