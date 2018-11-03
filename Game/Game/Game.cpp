#include "Game.h"

om_internal void
GameOutputSound(game_sound_output_buffer *SoundBuffer, int ToneHz)
{
	om_local_persist r32 tSine;
	i16 ToneVolume = 3000;
	int WavePeriod = SoundBuffer->SamplesPerSecond / ToneHz;

	i16 *SampleOut = SoundBuffer->Samples;
	for (int SampleIndex = 0; SampleIndex < SoundBuffer->SampleCount; ++SampleIndex)
	{
		r32 SineValue = sinf(tSine);
		i16 SampleValue = (i16)(SineValue * ToneVolume);
		*SampleOut++ = SampleValue;
		*SampleOut++ = SampleValue;

		tSine += 2.0f * OM_PI32 * 1.0f / (r32)WavePeriod;
	}
}

om_internal void
RenderGradient(game_offscreen_buffer *Buffer, int BlueOffset, int RedOffset)
{
	u8 *Row = (u8 *)Buffer->Memory;
	for (int Y = 0; Y < Buffer->Height; ++Y)
	{
		u32 *Pixel = (u32 *)Row;
		for (int X = 0; X < Buffer->Width; ++X)
		{
			u8 Blue = (u8)(X + BlueOffset);
			u8 Red = (u8)(Y + RedOffset);

			*Pixel++ = ((Red << 16) | Blue);
		}

		Row += Buffer->Pitch;
	}
}

om_internal void
GameUpdateAndRender(game_memory *Memory,
	game_input *Input, game_offscreen_buffer *Buffer)
{
	OM_ASSERT(sizeof(game_state) <= Memory->PermanentStorageSize);

	game_state *GameState = (game_state *)Memory->PermanentStorage;
	if (!Memory->IsInitialized)
	{
		char test[] = "C:\\Users\\Oskar\\Documents\\GitHub\\game\\README.md";
		char test2[] = "C:\\Users\\Oskar\\Documents\\GitHub\\game\\README2.md";
		char *FileName = test;
		char *TestFile = test2;
		
		debug_read_file_result BitmapMemory = DEBUGPlatformReadEntireFile(FileName);
		if (BitmapMemory.Contents)
		{
			DEBUGPlatformWriteEntireFile(TestFile, BitmapMemory.ContentsSize, BitmapMemory.Contents);
			DEBUGPlatformFreeFileMemory(BitmapMemory.Contents);
		}

		GameState->ToneHz = 256;

		//TODO: This may be more appropriate to let the platform layer do.
		Memory->IsInitialized = true;
	}

	for (int ControllerIndex = 0; ControllerIndex < OM_ARRAYCOUNT(Input->Controllers); ++ControllerIndex)
	{
		game_controller_input *Controller = GetController(Input, ControllerIndex);
		
		if (Controller->IsAnalog)
		{
			GameState->BlueOffset += (int)(4.0f*(Controller->StickAverageX));
			GameState->ToneHz = 256 + (int)(128.0f*(Controller->StickAverageY));
		}
		else
		{
			if (Controller->MoveLeft.EndedDown)
			{
				GameState->BlueOffset -= 1;
			}
			if (Controller->MoveRight.EndedDown)
			{
				GameState->BlueOffset += 1;
			}
		}
	}

	//TODO: Allow sample offsets here for more robust platform options
	//GameOutputSound(SoundBuffer, GameState->ToneHz);
	RenderGradient(Buffer, GameState->BlueOffset, GameState->RedOffset);
}

om_internal void
GameGetSoundSamples(game_memory *Memory, game_sound_output_buffer *SoundBuffer)
{
	game_state *GameState = (game_state *)Memory->PermanentStorage;
	GameOutputSound(SoundBuffer, GameState->ToneHz);
}