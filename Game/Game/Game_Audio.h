#ifndef GAME_AUDIO_H
#define GAME_AUDIO_H
#pragma once

struct playing_sound
{
	r32 Volume[2];
	sound_id ID;
	i32 SamplesPlayed;
	playing_sound *Next;
};

struct audio_state
{
	memory_arena *AudioArena;

	playing_sound *FirstPlayingSound;
	playing_sound *FirstFreePlayingSound;
};

#endif // GAME_AUDIO_H
