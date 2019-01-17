
om_internal void
DEBUGGameOutputSineWave(game_state *GameState, game_sound_output_buffer *SoundBuffer, int ToneHz)
{
	i16 ToneVolume = 3000;
	int WavePeriod = SoundBuffer->SamplesPerSecond / ToneHz;

	i16 *SampleOut = SoundBuffer->Samples;
	for (int SampleIndex = 0; SampleIndex < SoundBuffer->SampleCount; ++SampleIndex)
	{
		r32 SineValue = sinf(GameState->tSine);
		i16 SampleValue = (i16)(SineValue * ToneVolume);
		
		*SampleOut++ = SampleValue;
		*SampleOut++ = SampleValue;

		GameState->tSine += OM_TAU32 * 1.0f / (r32)WavePeriod;
		if (GameState->tSine > OM_TAU32)
		{
			GameState->tSine -= OM_TAU32;
		}
	}
}

om_internal playing_sound *
PlaySoundID(audio_state *AudioState, sound_id SoundID)
{
	if (!AudioState->FirstFreePlayingSound)
	{
		AudioState->FirstFreePlayingSound = PushStruct(AudioState->AudioArena, playing_sound);
		AudioState->FirstFreePlayingSound->Next = 0;
	}

	playing_sound *PlayingSound = AudioState->FirstFreePlayingSound;
	AudioState->FirstFreePlayingSound = PlayingSound->Next;

	PlayingSound->SamplesPlayed = 0;
	PlayingSound->CurrentVolume = PlayingSound->TargetVolume = Vector2(1.0f, 1.0f);
	PlayingSound->dCurrentVolume = Vector2(0.0f, 0.0f);
	PlayingSound->ID = SoundID;

	PlayingSound->Next = AudioState->FirstPlayingSound;
	AudioState->FirstPlayingSound = PlayingSound;

	return(PlayingSound);
}

om_internal void
ChangeVolume(audio_state *AudioState, playing_sound *Sound, r32 FadeDurationInSeconds, vector2 Volume)
{
	if (FadeDurationInSeconds <= 0.0f)
	{
		Sound->dCurrentVolume = Sound->TargetVolume = Volume;
	}
	else
	{
		r32 OneOverFade = 1.0f / FadeDurationInSeconds;
		Sound->TargetVolume = Volume;
		Sound->dCurrentVolume = OneOverFade * (Sound->TargetVolume - Sound->CurrentVolume);
	}
}

om_internal void
OutputMixedSounds(audio_state *AudioState, game_sound_output_buffer *SoundBuffer, game_assets *Assets, memory_arena *TemporaryArena)
{
	temporary_memory SoundMixerMemory = CreateTemporaryMemory(TemporaryArena);

	r32 *Channel0 = PushArray(TemporaryArena, SoundBuffer->SampleCount, r32);
	r32 *Channel1 = PushArray(TemporaryArena, SoundBuffer->SampleCount, r32);

	r32 SecondsPerSample = 1.0f / (r32)SoundBuffer->SamplesPerSecond;
#define AudioStateOutputChannelCount 2

	//Note:	Clearing the mixer channels.
	{
		r32 *Destination0 = Channel0;
		r32 *Destination1 = Channel1;

		for (int SampleIndex = 0; SampleIndex < SoundBuffer->SampleCount; ++SampleIndex)
		{
			*Destination0++ = 0.0f;
			*Destination1++ = 0.0f;
		}
	}

	//Note: Add all sounds together.
	for (playing_sound **PlayingSoundPointer = &AudioState->FirstPlayingSound; *PlayingSoundPointer;)
	{
		playing_sound *PlayingSound = *PlayingSoundPointer;
		b32 SoundFinished = false;

		u32 TotalSamplesToMix = SoundBuffer->SampleCount;
		r32 *Destination0 = Channel0;
		r32 *Destination1 = Channel1;

		while (TotalSamplesToMix && !SoundFinished)
		{
			loaded_sound *LoadedSound = GetSound(Assets, PlayingSound->ID);
			if (LoadedSound)
			{
				asset_sound_info *Info = GetSoundInfo(Assets, PlayingSound->ID);
				LoadSound(Assets, Info->NextIDToPlay);

				vector2 Volume = PlayingSound->CurrentVolume;
				vector2 dVolume = SecondsPerSample * PlayingSound->dCurrentVolume;

				OM_ASSERT(PlayingSound->SamplesPlayed >= 0);

				u32 SamplesToMix = TotalSamplesToMix;
				u32 SamplesRemainingInSound = LoadedSound->SampleCount - PlayingSound->SamplesPlayed;
				if (SamplesToMix > SamplesRemainingInSound)
				{
					SamplesToMix = SamplesRemainingInSound;
				}

				b32 VolumeEnded[AudioStateOutputChannelCount] = {};
				for (u32 ChannelIndex = 0; ChannelIndex < OM_ARRAYCOUNT(VolumeEnded); ++ChannelIndex)
				{
					if (dVolume.E[ChannelIndex] != 0.0f)
					{
						r32 DeltaVolume = (PlayingSound->TargetVolume.E[ChannelIndex] - Volume.E[ChannelIndex]);
						u32 VolumeSampleCount = (u32)((DeltaVolume / dVolume.E[ChannelIndex]) + 0.5f);
						if (SamplesToMix > VolumeSampleCount)
						{
							SamplesToMix = VolumeSampleCount;
							VolumeEnded[ChannelIndex] = true;
						}
					}
				}

				for (u32 SampleIndex = PlayingSound->SamplesPlayed; SampleIndex < (PlayingSound->SamplesPlayed + SamplesToMix); ++SampleIndex)
				{
					//TODO: Add support for stereo
					r32 SampleValue = LoadedSound->Samples[0][SampleIndex];
					*Destination0++ += AudioState->MasterVolume.E[0] * Volume.E[0] * SampleValue;
					*Destination1++ += AudioState->MasterVolume.E[1] * Volume.E[1] * SampleValue;

					Volume += dVolume;
				}

				PlayingSound->CurrentVolume = Volume;

				for (u32 ChannelIndex = 0; ChannelIndex < OM_ARRAYCOUNT(VolumeEnded); ++ChannelIndex)
				{
					if (VolumeEnded[ChannelIndex])
					{
						PlayingSound->CurrentVolume.E[ChannelIndex] = PlayingSound->TargetVolume.E[ChannelIndex];
						PlayingSound->dCurrentVolume.E[ChannelIndex] = 0.0f;
					}
				}

				OM_ASSERT(TotalSamplesToMix >= SamplesToMix);
				PlayingSound->SamplesPlayed += SamplesToMix;
				TotalSamplesToMix -= SamplesToMix;

				if ((u32)PlayingSound->SamplesPlayed == LoadedSound->SampleCount)
				{
					if (IsValid(Info->NextIDToPlay))
					{
						PlayingSound->ID = Info->NextIDToPlay;
						PlayingSound->SamplesPlayed = 0;
					}
					else
					{
						SoundFinished = true;
					}
				}
			}
			else
			{
				LoadSound(Assets, PlayingSound->ID);
				break;
			}
		}

		if (SoundFinished)
		{
			*PlayingSoundPointer = PlayingSound->Next;
			PlayingSound->Next = AudioState->FirstFreePlayingSound;
			AudioState->FirstFreePlayingSound = PlayingSound;
		}
		else
		{
			PlayingSoundPointer = &PlayingSound->Next;
		}
	}

	//Note: Converting the buffer values into 16 bit.
	{
		r32 *Source0 = Channel0;
		r32 *Source1 = Channel1;

		i16 *SampleOut = SoundBuffer->Samples;
		for (int SampleIndex = 0; SampleIndex < SoundBuffer->SampleCount; ++SampleIndex)
		{
			// TODO: Clamp once in SIMD
			*SampleOut++ = (i16)(*Source0++ + 0.5f);
			*SampleOut++ = (i16)(*Source1++ + 0.5f);
		}
	}

	DestroyTemporaryMemory(SoundMixerMemory);
}

om_internal void
InitializeAudioState(audio_state *AudioState, memory_arena *AudioArena)
{
	AudioState->AudioArena = AudioArena;
	AudioState->FirstPlayingSound = 0;
	AudioState->FirstFreePlayingSound = 0;
	AudioState->MasterVolume = Vector2(1.0f, 1.0f);
}