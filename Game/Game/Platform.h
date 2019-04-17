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
	i32 Pitch;
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

enum
{
	DebugCycleCounter_GameUpdateAndRender,
	DebugCycleCounter_RenderToBuffer,
	DebugCycleCounter_DEBUGDrawTransformedBitmap,
	DebugCycleCounter_SoftwareDrawTransformedBitmap,
	DebugCycleCounter_Count
};
typedef struct debug_cycle_counter
{
	u64 CycleCount;
	u32 HitCount;
} debug_cycle_counter;

extern struct game_memory *DebugGlobalMemory;
#if _MSC_VER
#define BEGIN_TIMED_BLOCK(ID) u64 StartCycleCount##ID = __rdtsc();
#define END_TIMED_BLOCK(ID) DebugGlobalMemory->Counters[DebugCycleCounter_##ID].CycleCount += __rdtsc() - StartCycleCount##ID; ++DebugGlobalMemory->Counters[DebugCycleCounter_##ID].HitCount;
#else
#define BEGIN_TIMED_BLOCK(ID) 
#define END_TIMED_BLOCK(ID) 
#endif

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

	// IMPORTANT: Samples must be padded to a multiple of 4 samples.
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

typedef struct platform_file_handle
{
	b32 HasErrors;
} platform_file_handle;

typedef struct platform_file_group
{
	u32 FileCount;
	void *Data;
} platform_file_group;

#define PLATFORM_GET_ALL_FILE_OF_TYPE_BEGIN(name) platform_file_group name(char *Type)
typedef PLATFORM_GET_ALL_FILE_OF_TYPE_BEGIN(platform_get_all_files_of_type_begin);

#define PLATFORM_GET_ALL_FILE_OF_TYPE_END(name) void name(platform_file_group FileGroup)
typedef PLATFORM_GET_ALL_FILE_OF_TYPE_END(platform_get_all_files_of_type_end);

#define PLATFORM_OPEN_FILE(name) platform_file_handle *name(platform_file_group FileGroup, u32 FileIndex)
typedef PLATFORM_OPEN_FILE(platform_open_file);

#define PLATFORM_READ_DATA_FROM_FILE(name) void name(platform_file_handle *Source, u64 Offset, u64 Size, void *Destination)
typedef PLATFORM_READ_DATA_FROM_FILE(platform_read_data_from_file);

#define PLATFORM_FILE_ERROR(name) void name(platform_file_handle *Handle, char *Message)
typedef PLATFORM_FILE_ERROR(platform_file_error);

#define PlatformNoFileErrors(Handle) (!(Handle)->HasErrors)

struct platform_thread_queue;
#define PLATFORM_THREAD_QUEUE_CALLBACK(name) void name(platform_thread_queue *Queue, void *Data)
typedef PLATFORM_THREAD_QUEUE_CALLBACK(platform_thread_queue_callback);

typedef void platform_add_thread_entry(platform_thread_queue *Queue, platform_thread_queue_callback *Callback, void *Data);
typedef void platform_complete_all_work(platform_thread_queue *Queue);

typedef struct platform_api
{
	platform_add_thread_entry *AddThreadEntry;
	platform_complete_all_work *CompleteAllThreadWork;

	platform_get_all_files_of_type_begin *GetAllFilesOfTypeBegin;
	platform_get_all_files_of_type_end *GetAllFilesOfTypeEnd;
	platform_open_file *OpenFile;
	platform_read_data_from_file *ReadDataFromFile;
	platform_file_error *FileError;

	debug_platform_read_entire_file *DEBUGReadEntireFile;
	debug_platform_free_file_memory *DEBUGFreeFileMemory;
	debug_platform_write_entire_file *DEBUGWriteEntireFile;
	debug_load_bitmap *DEBUGLoadBitmap;
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
	u64 PermanentStorageSize;
	void *PermanentStorage;			//NOTE: Required to be cleared to zero at startup.

	u64 TransientStorageSize;		//NOTE: Required to be cleared to zero at startup.
	void *TransientStorage;

	platform_thread_queue *HighPriorityQueue;
	platform_thread_queue *LowPriorityQueue;

	platform_api PlatformAPI;

#if 1 //TODO: Add actual define to use for enabling / Disabling this.
	debug_cycle_counter Counters[DebugCycleCounter_Count];
#endif
};

#define GAME_UPDATE_AND_RENDER(name) void name(game_memory *Memory, game_input *Input, game_offscreen_buffer *Buffer)
typedef GAME_UPDATE_AND_RENDER(game_update_and_render);

#define GAME_GET_SOUND_SAMPLES(name) void name(game_memory *Memory, game_sound_output_buffer *SoundBuffer)
typedef GAME_GET_SOUND_SAMPLES(game_get_sound_samples);

#endif // PLATFORM_H