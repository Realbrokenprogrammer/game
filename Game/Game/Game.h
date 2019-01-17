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
#include "Game_Asset.h"
#include "Game_Audio.h"

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
	b32 Initialized;

	memory_arena WorldArena;
	world *World;

	entity *ControlledEntity;

	entity *Player;
	entity *Player2;

	camera Camera;

	r32 Time;
	r32 tSine;

	audio_state AudioState;
	playing_sound *Music;
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

	game_assets *Assets;

	platform_thread_queue *HighPriorityQueue;
	platform_thread_queue *LowPriorityQueue;
};

om_global_variable platform_add_thread_entry *PlatformAddThreadEntry;
om_global_variable platform_complete_all_work *PlatformCompleteAllThreadWork;
om_global_variable debug_platform_read_entire_file *DEBUGPlatformReadEntireFile;

om_internal task_with_memory *BeginTaskWithMemory(transient_state *TransientState);
om_internal void EndTaskWithMemory(task_with_memory *Task);

#endif // GAME_H