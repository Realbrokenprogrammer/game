
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
	if (Sound)
	{
		if (FadeDurationInSeconds <= 0.0f)
		{
			Sound->CurrentVolume = Sound->TargetVolume = Volume;
		}
		else
		{
			r32 OneOverFade = 1.0f / FadeDurationInSeconds;
			Sound->TargetVolume = Volume;
			Sound->dCurrentVolume = OneOverFade * (Sound->TargetVolume - Sound->CurrentVolume);
		}
	}
}

om_internal void
ChangePitch(audio_state *AudioState, playing_sound *Sound, r32 dSample)
{
	if (Sound)
	{
		Sound->dSample = dSample;
	}
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

	OM_ASSERT((SoundBuffer->SampleCount & 3) == 0);
	u32 ChunkCount = SoundBuffer->SampleCount / 4;

	__m128 *Channel0 = PushArray(TemporaryArena, ChunkCount, __m128, 16);
	__m128 *Channel1 = PushArray(TemporaryArena, ChunkCount, __m128, 16);

	r32 SecondsPerSample = 1.0f / (r32)SoundBuffer->SamplesPerSecond;
#define AudioStateOutputChannelCount 2

	__m128 One_x4 = _mm_set1_ps(1.0f);
	__m128 Zero_x4 = _mm_set1_ps(0.0f);

	//Note:	Clearing the mixer channels.
	{
		__m128 *Destination0 = Channel0;
		__m128 *Destination1 = Channel1;

		for (u32 SampleIndex = 0; SampleIndex < ChunkCount; ++SampleIndex)
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

		u32 TotalChunksToMix = ChunkCount;
		__m128 *Destination0 = Channel0;
		__m128 *Destination1 = Channel1;

		while (TotalChunksToMix && !SoundFinished)
		{
			loaded_sound *LoadedSound = GetSound(Assets, PlayingSound->ID);
			if (LoadedSound)
			{
				asset_sound_info *Info = GetSoundInfo(Assets, PlayingSound->ID);
				PrefetchSound(Assets, Info->NextIDToPlay);

				vector2 Volume = PlayingSound->CurrentVolume;
				vector2 dVolume = SecondsPerSample * PlayingSound->dCurrentVolume;
				vector2 dVolumeChunk = 4.0f*dVolume;

				r32 dSample = PlayingSound->dSample;// *1.9f;
				r32 dSampleChunk = 4.0f*dSample;

				// Output channel 0
				__m128 MasterVolume0 = _mm_set1_ps(AudioState->MasterVolume.E[0]);
				__m128 Volume0 = _mm_setr_ps(Volume.E[0] + 0.0f*dVolume.E[0],
											 Volume.E[0] + 1.0f*dVolume.E[0],
											 Volume.E[0] + 2.0f*dVolume.E[0],
											 Volume.E[0] + 3.0f*dVolume.E[0]);
				__m128 dVolume0 = _mm_set1_ps(dVolume.E[0]);
				__m128 dVolumeChunk0 = _mm_set1_ps(dVolumeChunk.E[0]);

				// Output channel 1
				__m128 MasterVolume1 = _mm_set1_ps(AudioState->MasterVolume.E[1]);
				__m128 Volume1 = _mm_setr_ps(Volume.E[1] + 0.0f*dVolume.E[1],
											 Volume.E[1] + 1.0f*dVolume.E[1],
											 Volume.E[1] + 2.0f*dVolume.E[1],
											 Volume.E[1] + 3.0f*dVolume.E[1]);
				__m128 dVolume1 = _mm_set1_ps(dVolume.E[1]);
				__m128 dVolumeChunk1 = _mm_set1_ps(dVolumeChunk.E[1]);

				OM_ASSERT(PlayingSound->SamplesPlayed >= 0.0f);

				u32 ChunksToMix = TotalChunksToMix;
				r32 RealChunksRemainingInSound = (LoadedSound->SampleCount - RoundReal32ToInt32(PlayingSound->SamplesPlayed)) / dSampleChunk;
				u32 ChunksRemainingInSound = RoundReal32ToInt32(RealChunksRemainingInSound);
				
				if (ChunksToMix > ChunksRemainingInSound)
				{
					ChunksToMix = ChunksRemainingInSound;
				}

				u32 VolumeEndedAt[AudioStateOutputChannelCount] = {};
				for (u32 ChannelIndex = 0; ChannelIndex < OM_ARRAYCOUNT(VolumeEndedAt); ++ChannelIndex)
				{
					if (dVolumeChunk.E[ChannelIndex] != 0.0f)
					{
						r32 DeltaVolume = (PlayingSound->TargetVolume.E[ChannelIndex] - Volume.E[ChannelIndex]);
						u32 VolumeChunkCount = (u32)(((DeltaVolume / dVolumeChunk.E[ChannelIndex]) + 0.5f));
						if (ChunksToMix > VolumeChunkCount)
						{
							ChunksToMix = VolumeChunkCount;
							VolumeEndedAt[ChannelIndex] = VolumeChunkCount;
						}
					}
				}

				//TODO: Handle stereo
				r32 BeginSamplePosition = PlayingSound->SamplesPlayed;
				r32 EndSamplePosition = BeginSamplePosition + ChunksToMix * dSampleChunk;
				r32 LoopIndexC = (EndSamplePosition - BeginSamplePosition) / (r32)ChunksToMix;
				for (u32 LoopIndex = 0; LoopIndex < ChunksToMix; ++LoopIndex)
				{
					r32 SamplePosition = BeginSamplePosition + LoopIndexC * (r32)LoopIndex;

#if 1
					__m128 SamplePos = _mm_setr_ps(SamplePosition + 0.0f*dSample,
												   SamplePosition + 1.0f*dSample,
												   SamplePosition + 2.0f*dSample,
												   SamplePosition + 3.0f*dSample);
					__m128i SampleIndex = _mm_cvttps_epi32(SamplePos);
					__m128 Frac = _mm_sub_ps(SamplePos, _mm_cvtepi32_ps(SampleIndex));

					__m128 SampleValueF = _mm_setr_ps(LoadedSound->Samples[0][((i32 *)&SampleIndex)[0]],
													  LoadedSound->Samples[0][((i32 *)&SampleIndex)[1]],
													  LoadedSound->Samples[0][((i32 *)&SampleIndex)[2]],
													  LoadedSound->Samples[0][((i32 *)&SampleIndex)[3]]);
					__m128 SampleValueC = _mm_setr_ps(LoadedSound->Samples[0][((i32 *)&SampleIndex)[0] + 1],
													  LoadedSound->Samples[0][((i32 *)&SampleIndex)[1] + 1],
													  LoadedSound->Samples[0][((i32 *)&SampleIndex)[2] + 1],
													  LoadedSound->Samples[0][((i32 *)&SampleIndex)[3] + 1]);

					__m128 SampleValue = _mm_add_ps(_mm_mul_ps(_mm_sub_ps(One_x4, Frac), SampleValueF),
													_mm_mul_ps(Frac, SampleValueC));
#else
					__m128 SampleValue = _mm_setr_ps(LoadedSound->Samples[0][RoundReal32ToInt32(SamplePosition + 0.0f*dSample)],
													   LoadedSound->Samples[0][RoundReal32ToInt32(SamplePosition + 1.0f*dSample)],
													   LoadedSound->Samples[0][RoundReal32ToInt32(SamplePosition + 2.0f*dSample)],
													   LoadedSound->Samples[0][RoundReal32ToInt32(SamplePosition + 3.0f*dSample)]);
#endif


					__m128 D0 = _mm_load_ps((float *)&Destination0[0]);
					__m128 D1 = _mm_load_ps((float *)&Destination1[0]);

					D0 = _mm_add_ps(D0, _mm_mul_ps(_mm_mul_ps(MasterVolume0, Volume0), SampleValue));
					D1 = _mm_add_ps(D1, _mm_mul_ps(_mm_mul_ps(MasterVolume1, Volume1), SampleValue));
					
					_mm_store_ps((float *)&Destination0[0], D0);
					_mm_store_ps((float *)&Destination1[0], D1);

					++Destination0;
					++Destination1;
					Volume0 = _mm_add_ps(Volume0, dVolumeChunk0);
					Volume1 = _mm_add_ps(Volume1, dVolumeChunk1);
				}

				PlayingSound->CurrentVolume.E[0] = ((r32 *)&Volume0)[0];
				PlayingSound->CurrentVolume.E[1] = ((r32 *)&Volume1)[1];

				for (u32 ChannelIndex = 0; ChannelIndex < OM_ARRAYCOUNT(VolumeEndedAt); ++ChannelIndex)
				{
					if (ChunksToMix == VolumeEndedAt[ChannelIndex])
					{
						PlayingSound->CurrentVolume.E[ChannelIndex] = PlayingSound->TargetVolume.E[ChannelIndex];
						PlayingSound->dCurrentVolume.E[ChannelIndex] = 0.0f;
					}
				}

				PlayingSound->SamplesPlayed = EndSamplePosition;
				OM_ASSERT(TotalChunksToMix >= ChunksToMix);
				TotalChunksToMix -= ChunksToMix;

				if (ChunksToMix == ChunksRemainingInSound)
				{
					if (IsValid(Info->NextIDToPlay))
					{
						PlayingSound->ID = Info->NextIDToPlay;
						OM_ASSERT(PlayingSound->SamplesPlayed >= LoadedSound->SampleCount);
						PlayingSound->SamplesPlayed -= (r32)LoadedSound->SampleCount;
						if (PlayingSound->SamplesPlayed < 0)
						{
							PlayingSound->SamplesPlayed = 0.0f;
						}
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
		for (u32 SampleIndex = 0; SampleIndex < ChunkCount; ++SampleIndex)
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