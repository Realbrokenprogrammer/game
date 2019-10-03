#ifndef GAME_H
#define GAME_H
#pragma once

#include "Platform.h"
#include "Game_Intristics.h"
#include "Game_Math.h"
#include "Game_File_Formats.h"
#include "Game_Memory.h"
#include "Game_Physics.h"
#include "Game_Entity.h"
#include "Game_World.h"
#include "Game_Camera.h"
#include "Game_Renderer.h"
#include "Game_Asset_Type_Id.h"
#include "Game_Asset.h"
#include "Game_Audio.h"
#include "Game_Random.h"

//TODO: This could be improved, think of the structure of this later.
enum game_mode
{
	GameMode_None,

	GameMode_Menu,
	GameMode_Cutscene,
	GameMode_World
};

struct particle_cell
{
	r32 Density;
	vector2 VelocityTimesDensity;
};

struct particle
{
	bitmap_id BitmapID;
	vector2 Position;
	vector2 dPosition;
	vector2 ddPosition;
	vector4 Color;
	vector4 dColor;
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

	random_sequence ParticlesEntropy; // NOTE: Entropy for the particles

#define PARTICLE_CELL_DIM 32
	u32 NextParticle;
	particle Particles[256];

	particle_cell ParticleCells[PARTICLE_CELL_DIM][PARTICLE_CELL_DIM];
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

om_global_variable platform_api Platform;

om_internal task_with_memory *BeginTaskWithMemory(transient_state *TransientState);
om_internal void EndTaskWithMemory(task_with_memory *Task);

#endif // GAME_H