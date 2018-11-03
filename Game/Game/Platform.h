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

#if OM_DEBUG
// These are NOT for doing anything in the release version of the game. They are blocking 
// and the write doesn't protect against lost data.
struct debug_read_file_result
{
	u32 ContentsSize;
	void *Contents;
};
om_internal debug_read_file_result DEBUGPlatformReadEntireFile(char *FileName);
om_internal void DEBUGPlatformFreeFileMemory(void *Memory);

om_internal b32 DEBUGPlatformWriteEntireFile(char *FileName, u32 MemorySize, void *Memory);

om_internal loaded_bitmap DEBUGLoadBitmap(char * FileName);
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
	//TODO: Insert clock value here.
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

#endif // PLATFORM_H