#ifndef GAME_AUDIO_H
#define GAME_AUDIO_H
#pragma once

struct playing_sound
{
	vector2 CurrentVolume;
	vector2 dCurrentVolume; //TODO: Naming.
	vector2 TargetVolume;

	r32 dSample;

	sound_id ID;
	r32 SamplesPlayed;
	playing_sound *Next;
};

struct audio_state
{
	memory_arena *AudioArena;

	playing_sound *FirstPlayingSound;
	playing_sound *FirstFreePlayingSound;

	vector2 MasterVolume;
};

#endif // GAME_AUDIO_H
