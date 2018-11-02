#ifndef GAME_H
#define GAME_H
#pragma once

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
	int RedOffset;
	int BlueOffset;
};

#endif // GAME_H