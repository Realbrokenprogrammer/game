#pragma once

#include <math.h>
#include "om_tool.h"

//TODO: SWAP, MIN, MAX ... Macros?

/*
	Services that the platform provides to the game
*/

// These are NOT for doing anything in the release version of the game. They are blocking 
// and the write doesn't protect against lost data.
struct debug_read_file_result
{
	u32 ContentsSize;
	void *Contents;
};
debug_read_file_result DEBUGPlatformReadEntireFile(char *FileName);
void DEBUGPlatformFreeFileMemory(void *Memory);

b32 DEBUGPlatformWriteEntireFile(char *FileName, u32 MemorySize, void *Memory);

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

	r32 StartX;
	r32 StartY;

	r32 MinX;
	r32 MinY;

	r32 MaxX;
	r32 MaxY;

	r32 EndY;
	r32 EndX;

	union
	{
		game_button_state Buttons[6];
		struct {
			game_button_state Up;
			game_button_state Down;
			game_button_state Left;
			game_button_state Right;
			game_button_state LeftShoulder;
			game_button_state RightShoulder;
		};
	};
};

struct game_input
{
	//TODO: Insert clock value here.
	game_controller_input Controllers[4];
};

struct game_memory
{
	b32 IsInitialized;

	u64 PermanentStorageSize;
	void *PermanentStorage;			//NOTE: Required to be cleared to zero at startup.

	u64 TransientStorageSize;
	void *TransientStorage;
};

om_internal void GameUpdateAndRender(game_memory *Memory,
	game_input *Input, game_offscreen_buffer *Buffer,
	game_sound_output_buffer *SoundBuffer);


struct game_state
{
	int ToneHz;
	int GreenOffset;
	int BlueOffset;
};