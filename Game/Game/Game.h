#ifndef GAME_H
#define GAME_H
#pragma once

#include "Game_Entity.h"
#include "Game_World.h"

struct game_memory
{
	b32 IsInitialized;

	u64 PermanentStorageSize;
	void *PermanentStorage;			//NOTE: Required to be cleared to zero at startup.

	u64 TransientStorageSize;
	void *TransientStorage;
};

om_internal void GameUpdateAndRender(game_memory *Memory,
									 game_input *Input, game_offscreen_buffer *Buffer);
om_internal void GameGetSoundSamples(game_memory *Memory, game_sound_output_buffer *SoundBuffer);

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

	world_position CameraPosition;

	int ToneHz;
	int RedOffset;
	int BlueOffset;

	loaded_bitmap PlayerBitmap;
	loaded_bitmap GrassBitmap;
	loaded_bitmap WaterBitmap;
};

#endif // GAME_H