
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
	PlayingSound->dSample = 1.0f;

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
ChangePitch(audio_state *AudioState, playing_sound *Sound, r32 dSample)
{
	Sound->dSample = dSample;
}

om_internal void
DEBUGOutputMixedSounds(audio_state *AudioState, game_sound_output_buffer *SoundBuffer, 
	game_assets *Assets, memory_arena *TemporaryArena)
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

				r32 dSample = PlayingSound->dSample;
				OM_ASSERT(PlayingSound->SamplesPlayed >= 0.0f);

				u32 SamplesToMix = TotalSamplesToMix;
				r32 RealSamplesRemainingInSound = (LoadedSound->SampleCount - RoundReal32ToInt32(PlayingSound->SamplesPlayed)) / dSample;
				u32 SamplesRemainingInSound = RoundReal32ToInt32(RealSamplesRemainingInSound);
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

				r32 SamplePosition = PlayingSound->SamplesPlayed;
				for (u32 LoopIndex = 0; LoopIndex < SamplesToMix; ++LoopIndex)
				{
#if 1
					u32 SampleIndex = FloorReal32ToInt32(SamplePosition);
					r32 Frac = SamplePosition - (r32)SampleIndex;
					r32 Sample0 = (r32)LoadedSound->Samples[0][SampleIndex];
					r32 Sample1 = (r32)LoadedSound->Samples[0][SampleIndex + 1];
					r32 SampleValue = Lerp(Sample0, Sample1, Frac);
#else
					u32 SampleIndex = RoundReal32ToInt32(SamplePosition);
					r32 SampleValue = LoadedSound->Samples[0][SampleIndex];
#endif

					//TODO: Add support for stereo
					*Destination0++ += AudioState->MasterVolume.E[0] * Volume.E[0] * SampleValue;
					*Destination1++ += AudioState->MasterVolume.E[1] * Volume.E[1] * SampleValue;

					Volume += dVolume;
					SamplePosition += dSample;
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
				PlayingSound->SamplesPlayed = SamplePosition;
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
OutputMixedSounds(audio_state *AudioState, game_sound_output_buffer *SoundBuffer, game_assets *Assets, memory_arena *TemporaryArena)
{
	temporary_memory SoundMixerMemory = CreateTemporaryMemory(TemporaryArena);

	//TODO: Remove
	//u32 SampleCountAlign4 = Align4(SoundBuffer->SampleCount);
	//u32 SampleCount4 = SampleCountAlign4 / 4;

	OM_ASSERT((SoundBuffer->SampleCount & 7) == 0);
	u32 SampleCount8 = SoundBuffer->SampleCount / 8;
	u32 SampleCount4 = SoundBuffer->SampleCount / 4;

	__m128 *Channel0 = PushArray(TemporaryArena, SoundBuffer->SampleCount, __m128, 16);
	__m128 *Channel1 = PushArray(TemporaryArena, SoundBuffer->SampleCount, __m128, 16);

	r32 SecondsPerSample = 1.0f / (r32)SoundBuffer->SamplesPerSecond;
#define AudioStateOutputChannelCount 2

	__m128 Zero_x4 = _mm_set1_ps(0.0f);

	//Note:	Clearing the mixer channels.
	{
		__m128 *Destination0 = Channel0;
		__m128 *Destination1 = Channel1;

		for (u32 SampleIndex = 0; SampleIndex < SampleCount4; ++SampleIndex)
		{
			_mm_store_ps((float *)Destination0++, Zero_x4);
			_mm_store_ps((float *)Destination1++, Zero_x4);
		}
	}

	//Note: Add all sounds together.
	for (playing_sound **PlayingSoundPointer = &AudioState->FirstPlayingSound; *PlayingSoundPointer;)
	{
		playing_sound *PlayingSound = *PlayingSoundPointer;
		b32 SoundFinished = false;

		u32 TotalSamplesToMix8 = SampleCount8;
		__m128 *Destination0 = Channel0;
		__m128 *Destination1 = Channel1;

		while (TotalSamplesToMix8 && !SoundFinished)
		{
			loaded_sound *LoadedSound = GetSound(Assets, PlayingSound->ID);
			if (LoadedSound)
			{
				asset_sound_info *Info = GetSoundInfo(Assets, PlayingSound->ID);
				LoadSound(Assets, Info->NextIDToPlay);

				vector2 Volume = PlayingSound->CurrentVolume;
				vector2 dVolume = SecondsPerSample * PlayingSound->dCurrentVolume;
				vector2 dVolume8 = 8.0f*dVolume;

				r32 dSample = PlayingSound->dSample;
				r32 dSample8 = 8.0f*dSample;

				__m128 MasterVolume4_0 = _mm_set1_ps(AudioState->MasterVolume.E[0]);
				__m128 MasterVolume4_1 = _mm_set1_ps(AudioState->MasterVolume.E[1]);
				__m128 Volume4_0 = _mm_setr_ps(Volume.E[0] + 0.0f*dVolume.E[0],
											   Volume.E[0] + 1.0f*dVolume.E[0],
											   Volume.E[0] + 2.0f*dVolume.E[0],
											   Volume.E[0] + 3.0f*dVolume.E[0]);
				__m128 dVolume4_0 = _mm_set1_ps(dVolume.E[0]);
				__m128 dVolume84_0 = _mm_set1_ps(dVolume8.E[0]);
				__m128 Volume4_1 = _mm_setr_ps(Volume.E[1] + 0.0f*dVolume.E[1],
											   Volume.E[1] + 1.0f*dVolume.E[1],
											   Volume.E[1] + 2.0f*dVolume.E[1],
											   Volume.E[1] + 3.0f*dVolume.E[1]);
				__m128 dVolume4_1 = _mm_set1_ps(dVolume.E[1]);
				__m128 dVolume84_1 = _mm_set1_ps(dVolume8.E[1]);

				OM_ASSERT(PlayingSound->SamplesPlayed >= 0.0f);

				u32 SamplesToMix8 = TotalSamplesToMix8;
				r32 RealSamplesRemainingInSound8 = (LoadedSound->SampleCount - RoundReal32ToInt32(PlayingSound->SamplesPlayed)) / dSample8;
				u32 SamplesRemainingInSound8 = RoundReal32ToInt32(RealSamplesRemainingInSound8);
				if (SamplesToMix8 > SamplesRemainingInSound8)
				{
					SamplesToMix8 = SamplesRemainingInSound8;
				}

				b32 VolumeEnded[AudioStateOutputChannelCount] = {};
				for (u32 ChannelIndex = 0; ChannelIndex < OM_ARRAYCOUNT(VolumeEnded); ++ChannelIndex)
				{
					// TODO: Fix bug regarding both volumes ending at the same time.
					if (dVolume8.E[ChannelIndex] != 0.0f)
					{
						r32 DeltaVolume = (PlayingSound->TargetVolume.E[ChannelIndex] - Volume.E[ChannelIndex]);
						u32 VolumeSampleCount8 = (u32)(((DeltaVolume / dVolume8.E[ChannelIndex]) + 0.5f));
						if (SamplesToMix8 > VolumeSampleCount8)
						{
							SamplesToMix8 = VolumeSampleCount8;
							VolumeEnded[ChannelIndex] = true;
						}
					}
				}

				r32 SamplePosition = PlayingSound->SamplesPlayed;
				for (u32 LoopIndex = 0; LoopIndex < SamplesToMix8; ++LoopIndex)
				{
#if 0
					r32 OffsetSamplePosition = SamplePosition + (r32)SampleOffset*dSample;
					u32 SampleIndex = FloorReal32ToInt32(SamplePosition);
					r32 Frac = SamplePosition - (r32)SampleIndex;
					
					r32 Sample0 = (r32)LoadedSound->Samples[0][SampleIndex];
					r32 Sample1 = (r32)LoadedSound->Samples[0][SampleIndex + 1];
					r32 SampleValue = Lerp(Sample0, Sample1, Frac);
#else
					__m128 SampleValue_0 = _mm_setr_ps(LoadedSound->Samples[0][RoundReal32ToInt32(SamplePosition + 0.0f*dSample)],
													   LoadedSound->Samples[0][RoundReal32ToInt32(SamplePosition + 1.0f*dSample)],
													   LoadedSound->Samples[0][RoundReal32ToInt32(SamplePosition + 2.0f*dSample)],
													   LoadedSound->Samples[0][RoundReal32ToInt32(SamplePosition + 3.0f*dSample)]);
					__m128 SampleValue_1 = _mm_setr_ps(LoadedSound->Samples[0][RoundReal32ToInt32(SamplePosition + 4.0f*dSample)],
													   LoadedSound->Samples[0][RoundReal32ToInt32(SamplePosition + 5.0f*dSample)],
													   LoadedSound->Samples[0][RoundReal32ToInt32(SamplePosition + 6.0f*dSample)],
													   LoadedSound->Samples[0][RoundReal32ToInt32(SamplePosition + 7.0f*dSample)]);
#endif


					__m128 D0_0 = _mm_load_ps((float *)&Destination0[0]);
					__m128 D0_1 = _mm_load_ps((float *)&Destination0[1]);
					__m128 D1_0 = _mm_load_ps((float *)&Destination1[0]);
					__m128 D1_1 = _mm_load_ps((float *)&Destination1[1]);

					D0_0 = _mm_add_ps(D0_0, _mm_mul_ps(_mm_mul_ps(MasterVolume4_0, Volume4_0), SampleValue_0));
					D0_1 = _mm_add_ps(D0_1, _mm_mul_ps(_mm_mul_ps(MasterVolume4_0, _mm_add_ps(dVolume4_0, Volume4_0)), SampleValue_1));
					D1_0 = _mm_add_ps(D1_0, _mm_mul_ps(_mm_mul_ps(MasterVolume4_1, Volume4_1), SampleValue_0));
					D1_1 = _mm_add_ps(D1_1, _mm_mul_ps(_mm_mul_ps(MasterVolume4_1, _mm_add_ps(dVolume4_1, Volume4_1)), SampleValue_1));

					_mm_store_ps((float *)&Destination0[0], D0_0);
					_mm_store_ps((float *)&Destination0[1], D0_1);
					_mm_store_ps((float *)&Destination1[0], D1_0);
					_mm_store_ps((float *)&Destination1[1], D1_1);

					Destination0 += 2;
					Destination1 += 2;
					Volume4_0 = _mm_add_ps(Volume4_0, dVolume84_0);
					Volume4_1 = _mm_add_ps(Volume4_1, dVolume84_1);
					Volume += dVolume8;
					SamplePosition += dSample8;
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

				PlayingSound->SamplesPlayed = SamplePosition;
				OM_ASSERT(TotalSamplesToMix8 >= SamplesToMix8);
				TotalSamplesToMix8 -= SamplesToMix8;

				if ((u32)PlayingSound->SamplesPlayed >= LoadedSound->SampleCount)
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
		__m128 *Source0 = Channel0;
		__m128 *Source1 = Channel1;

		__m128i *SampleOut = (__m128i *)SoundBuffer->Samples;
		for (u32 SampleIndex = 0; SampleIndex < SampleCount4; ++SampleIndex)
		{
			__m128 S0 = _mm_load_ps((float *)Source0++);
			__m128 S1 = _mm_load_ps((float *)Source1++);

			__m128i L = _mm_cvtps_epi32(S0);
			__m128i R = _mm_cvtps_epi32(S1);

			__m128i LR0 = _mm_unpacklo_epi32(L, R);
			__m128i LR1 = _mm_unpackhi_epi32(L, R);

			__m128i S01 = _mm_packs_epi32(LR0, LR1);

			*SampleOut++ = S01;
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