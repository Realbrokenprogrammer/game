#include "Game.h"
#include "Game_World.cpp"

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
DrawRect(game_offscreen_buffer *Buffer, vector2 Min, vector2 Max, r32 R, r32 G, r32 B)
{
	i32 MinX = RoundReal32ToInt32(Min.x);
	i32 MinY = RoundReal32ToInt32(Min.y);
	i32 MaxX = RoundReal32ToInt32(Max.x);
	i32 MaxY = RoundReal32ToInt32(Max.y);

	if (MinX < 0)
	{
		MinX = 0;
	}

	if (MinY < 0)
	{
		MinY = 0;
	}

	if (MaxX > Buffer->Width)
	{
		MaxX = Buffer->Width;
	}

	if (MaxY > Buffer->Height)
	{
		MaxY = Buffer->Height;
	}

	u32 Color = 
		((RoundReal32ToInt32(R * 255.0f) << 16) | 
		(RoundReal32ToInt32(G * 255.0f) << 8) | 
		(RoundReal32ToInt32(B * 255.0f)));

	u8 *Row = ((u8 *)Buffer->Memory + MinX*Buffer->BytesPerPixel + MinY * Buffer->Pitch);
	for (int Y = MinY; Y < MaxY; ++Y)
	{
		u32 *Pixel = (u32 *)Row;
		for (int X = MinX; X < MaxX; ++X)
		{
			*Pixel++ = Color;
		}

		Row += Buffer->Pitch;
	}
}

om_internal void
DrawBitmap(game_offscreen_buffer *Buffer, loaded_bitmap *Bitmap, r32 TargetX, r32 TargetY, r32 ColorAlpha)
{
	i32 MinX = RoundReal32ToInt32(TargetX);
	i32 MinY = RoundReal32ToInt32(TargetY);
	i32 MaxX = MinX + Bitmap->Width;
	i32 MaxY = MinY + Bitmap->Height;

	i32 SourceOffsetX = 0;
	if (MinX < 0)
	{
		SourceOffsetX = -MinX;
		MinX = 0;
	}

	i32 SourceOffsetY = 0;
	if (MinY < 0)
	{
		SourceOffsetY = -MinY;
		MinY = 0;
	}

	if (MaxX > Buffer->Width)
	{
		MaxX = Buffer->Width;
	}

	if (MaxY > Buffer->Height)
	{
		MaxY = Buffer->Height;
	}

	u32 *Source = Bitmap->Pixels;
	u8 *Row = ((u8 *)Buffer->Memory + MinX * Buffer->BytesPerPixel + MinY * Buffer->Pitch);
	for (int Y = MinY; Y < MaxY; ++Y)
	{
		u32 *Pixel = (u32 *)Row;
		for (int X = MinX; X < MaxX; ++X)
		{
			*Pixel++ = *Source++;
		}

		Row += Buffer->Pitch;
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

inline vector2
GetCamera(game_state *GameState)
{
	world_difference Diff = SubtractPosition2D(GameState->World, 0, &GameState->CameraPosition);
	vector2 Result = Diff.deltaXY;

	return (Result);
}

om_internal void 
SetCamera()
{

}

om_internal void
AddPlayer()
{

}

//TODO: Perhaps don't return pointer to the entity?
om_internal entity *
AddEntity(game_state *GameState, entity_type Type, world_position *Position)
{
	//TODO: Assert that we're not adding more entities than what the GameState can hold.
	u32 EntityIndex = GameState->EntityCount++;

	entity *Entity = GameState->Entities + EntityIndex;
	*Entity = {};
	Entity->Type = Type;

	ChangeEntityLocation(GameState->World, EntityIndex, Entity, 0, Position);

	return (Entity);
}

om_internal entity
AddWall(game_state *GameState, u32 PosX, u32 PosY)
{
	//TODO: The Position times the constant value is temporary. Remove this when entities got position representation
	// within the world tiles.
	world_position Position = {(r32)PosX * 70, (r32)PosY * 80}; // Get position based on tile x and y
	entity Entity = *AddEntity(GameState, EntityType_Wall, &Position);

	return (Entity);
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

		GameState->EntityCount = 0;

		// Initializing World
		world *World = GameState->World;
		if (World == NULL)
		{
			// TODO: Temporary call. Don't use Virtual Alloc here and let Platform layer handle allocation.
			World = (world *)VirtualAlloc(0, sizeof(world), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
		}
		InitializeWorld(World, 1.4);

		u32 TilesPerWidth = 17;
		u32 TilesPerHeight = 9;

		for (u32 TileY = 0; TileY < TilesPerHeight; ++TileY)
		{
			for (u32 TileX = 0; TileX < TilesPerWidth; ++TileX)
			{
				u32 TileValue = 1;

				if ((TileX == 0) || (TileY == 0) || (TileY == (TilesPerHeight -1)) || (TileX == (TilesPerWidth -1)))
				{
					TileValue = 2;
				}

				if (TileValue == 2)
				{
					AddWall(GameState, TileX, TileY);
				}

			}
		}

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
	/*RenderGradient(Buffer, GameState->BlueOffset, GameState->RedOffset);

	vector2 min = Vector2(150, 150);
	vector2 max = Vector2(200, 200);
	DrawRect(Buffer, Vector2(0.0f, 0.0f), Vector2((r32)Buffer->Width, (r32)Buffer->Height), 1.0f, 0.0f, 0.0f);
	DrawRect(Buffer, min, max, 0.0f, 0.0f, 1.0f);


	DrawBitmap(Buffer, &GameState->Bitmap, 500, 300, 0.0f);*/

	for (u32 EntityIndex = 0; EntityIndex < GameState->EntityCount; ++EntityIndex)
	{
		entity Entity = GameState->Entities[EntityIndex];

		switch (Entity.Type)
		{
			case EntityType_Wall:
			{
				DrawBitmap(Buffer, &GameState->Bitmap, Entity.Position.x, Entity.Position.y, 0.0f);
			} break;
			case EntityType_Null:
				break;
			default:
				break;
		}
	}
}

om_internal void
GameGetSoundSamples(game_memory *Memory, game_sound_output_buffer *SoundBuffer)
{
	game_state *GameState = (game_state *)Memory->PermanentStorage;
	GameOutputSound(SoundBuffer, GameState->ToneHz);
}