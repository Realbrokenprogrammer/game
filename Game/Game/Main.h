#ifndef MAIN_H
#define MAIN_H
#pragma once

struct win32_offscreen_buffer
{
	// NOTE: Pixels are alwasy 32-bits wide, Memory Order BB GG RR XX
	BITMAPINFO Info;
	void *Memory;
	int Width;
	int Height;
	int Pitch;
	int BytesPerPixel;
};

struct win32_window_dimension
{
	int Width;
	int Height;
};

struct win32_sound_output
{
	int SamplesPerSecond;
	u32 RunningSampleIndex;
	int BytesPerSample;
	DWORD SecondaryBufferSize;
	DWORD SafetyBytes;

	// TODO: Should running sample index be in bytes as well
	// TODO: Math gets simpler if we add a "bytes per second" field?
};

struct win32_debug_time_marker
{
	DWORD OutputPlayCursor;
	DWORD OutputWriteCursor;
	DWORD OutputLocation;
	DWORD OutputByteCount;
	DWORD ExpectedFlipPlayCursor;

	DWORD FlipPlayCursor;
	DWORD FlipWriteCursor;
};

struct win32_game_code
{
	HMODULE GameCodeDLL;
	FILETIME DLLLastWriteTime;

	// IMPORTANT: Either of the callbacks can be 0!  You must
	// check before calling.
	game_update_and_render *UpdateAndRender;
	game_get_sound_samples *GetSoundSamples;

	b32 IsValid;
};

#define WIN32_STATE_FILE_NAME_COUNT MAX_PATH
struct win32_replay_buffer
{
	HANDLE FileHandle;
	HANDLE MemoryMap;
	char FileName[WIN32_STATE_FILE_NAME_COUNT];
	void *MemoryBlock;
};

struct win32_state
{
	u64 TotalSize;
	void *GameMemoryBlock;
	win32_replay_buffer ReplayBuffers[4];

	HANDLE RecordingHandle;
	int InputRecordingIndex;

	HANDLE PlaybackHandle;
	int InputPlayingIndex;

	char EXEFileName[WIN32_STATE_FILE_NAME_COUNT];
	char *OnePastLastEXEFileNameSlash;
};

struct sdl_offscreen_buffer
{
	//SDL_Texture *Texture;
	void *Memory;
	int Width;
	int Height;
	int Pitch;
	int BytesPerPixel;
};

struct sdl_window_dimension
{
	int Width;
	int Height;
};

struct sdl_audio_ring_buffer
{
	int Size;
	int WriteCursor;
	int PlayCursor;
	void *Data;
};

struct sdl_debug_time_marker
{
	int PlayCursor;
	int WriteCursor;
};

struct sdl_sound_output
{
	int SamplesPerSecond;
	u32 RunningSampleIndex;
	int BytesPerSample;
	int SecondaryBufferSize;
	r32 tSine;
	u32 SafetyBytes;
};

struct sdl_game_code
{
	HMODULE GameCodeDLL;
	FILETIME DLLLastWriteTime;

	game_update_and_render *UpdateAndRender;
	game_get_sound_samples *GetSoundSamples;

	b32 IsValid;
};

struct sdl_state
{
	u64 TotalSize;
	void *GameMemoryBlock;

	char EXEFileName[MAX_PATH];
	char *OnePastLastEXEFileNameSlash;
};

#endif // MAIN_H