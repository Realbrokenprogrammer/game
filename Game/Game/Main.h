#ifndef MAIN_H
#define MAIN_H
#pragma once

struct sdl_offscreen_buffer
{
	SDL_Texture *Texture;
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
	game_update_and_render *UpdateAndRender;
	game_get_sound_samples *GetSoundSamples;

	b32 IsValid;
};

#endif // MAIN_H