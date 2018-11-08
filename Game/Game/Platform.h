#ifndef PLATFORM_H
#define PLATFORM_H
#pragma once

#include "Types.h"

/*
	Services that the platform provides to the game
*/

struct loaded_bitmap
{
	i32 Width;
	i32 Height;
	u32 *Pixels;
};

#if 1 //TODO: Add compiler flag for this define OM_DEBUG
// These are NOT for doing anything in the release version of the game. They are blocking 
// and the write doesn't protect against lost data.
struct debug_read_file_result
{
	u32 ContentsSize;
	void *Contents;
};

//debug_read_file_result DEBUGPlatformReadEntireFile(char *FileName);
#define DEBUG_PLATFORM_READ_ENTIRE_FILE(name) debug_read_file_result name(char *FileName)
typedef DEBUG_PLATFORM_READ_ENTIRE_FILE(debug_platform_read_entire_file);


//void DEBUGPlatformFreeFileMemory(void *Memory);
#define DEBUG_PLATFORM_FREE_FILE_MEMORY(name) void name(void *Memory)
typedef DEBUG_PLATFORM_FREE_FILE_MEMORY(debug_platform_free_file_memory);

//b32 DEBUGPlatformWriteEntireFile(char *FileName, u32 MemorySize, void *Memory);
#define DEBUG_PLATFORM_WRITE_ENTIRE_FILE(name) b32 name(char *FileName, u32 MemorySize, void *Memory)
typedef DEBUG_PLATFORM_WRITE_ENTIRE_FILE(debug_platform_write_entire_file);

//loaded_bitmap DEBUGLoadBitmap(char * FileName);
#define DEBUG_LOAD_BITMAP(name) loaded_bitmap name(char *FileName)
typedef DEBUG_LOAD_BITMAP(debug_load_bitmap);

#endif

/*
	Services that the game provides to the platform
*/

struct game_offscreen_buffer
{
	//Note: Pixels are in 32-bits wide, Memory Order BB GG RR XX
	void *Memory;
	int Width;
	int Height;
	int Pitch;
	int BytesPerPixel;
};

struct game_sound_output_buffer
{
	int SamplesPerSecond;
	int SampleCount;
	i16 *Samples;
};

struct game_button_state
{
	int HalfTransitionCount;
	b32 EndedDown;
};

struct game_controller_input
{
	b32 IsConnected;
	b32 IsAnalog;

	r32 StickAverageX;
	r32 StickAverageY;

	union
	{
		game_button_state Buttons[12];
		struct {
			game_button_state MoveUp;
			game_button_state MoveDown;
			game_button_state MoveLeft;
			game_button_state MoveRight;

			game_button_state ActionUp;
			game_button_state ActionDown;
			game_button_state ActionLeft;
			game_button_state ActionRight;

			game_button_state LeftShoulder;
			game_button_state RightShoulder;

			game_button_state Start;
			game_button_state Back;
		};
	};
};

struct game_input
{
	r32 dtForFrame;

	game_controller_input Controllers[5];
};

inline game_controller_input *
GetController(game_input *Input, int ControllerIndex)
{
	OM_ASSERT(ControllerIndex < OM_ARRAYCOUNT(Input->Controllers));
	game_controller_input *Result = &Input->Controllers[ControllerIndex];

	return (Result);
}

inline b32
WasPressed(game_button_state State)
{
	b32 Result = ((State.HalfTransitionCount > 1) ||
		((State.HalfTransitionCount == 1) && (State.EndedDown)));

	return (Result);
}

inline b32
IsDown(game_button_state State)
{
	b32 Result = (State.EndedDown);

	return (Result);
}

struct game_memory
{
	b32 IsInitialized;

	u64 PermanentStorageSize;
	void *PermanentStorage;			//NOTE: Required to be cleared to zero at startup.

	u64 TransientStorageSize;
	void *TransientStorage;

	debug_platform_read_entire_file *DEBUGPlatformReadEntireFile;
	debug_platform_free_file_memory *DEBUGPlatformFreeFileMemory;
	debug_platform_write_entire_file *DEBUGPlatformWriteEntireFile;
	debug_load_bitmap *DEBUGLoadBitmap;
};

#define GAME_UPDATE_AND_RENDER(name) void name(game_memory *Memory, game_input *Input, game_offscreen_buffer *Buffer)
typedef GAME_UPDATE_AND_RENDER(game_update_and_render);
GAME_UPDATE_AND_RENDER(GameUpdateAndRenderStub)
{
}

#define GAME_GET_SOUND_SAMPLES(name) void name(game_memory *Memory, game_sound_output_buffer *SoundBuffer)
typedef GAME_GET_SOUND_SAMPLES(game_get_sound_samples);
GAME_GET_SOUND_SAMPLES(GameGetSoundSamplesStub)
{
}

#endif // PLATFORM_H