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

}

om_internal void 
SetCamera()
{

}

//TODO: Perhaps don't return pointer to the entity?
om_internal entity *
AddEntity(world_layer *Layer, entity_type Type, world_position *Position)
{
	//TODO: Assert that we're not adding more entities than what the GameState can hold.
	u32 EntityIndex = Layer->EntityCount++;

	entity *Entity = Layer->Entities + EntityIndex;
	*Entity = {};
	Entity->Type = Type;
	Entity->Position = {(r32)Position->X, (r32)Position->Y};

	return (Entity);
}

//TODO: Should return reference or not?
om_internal entity
AddPlayer(world_layer *Layer, u32 PositionX, u32 PositionY)
{
	world_position Position = { PositionX, PositionY, 0 };
	entity *Entity = AddEntity(Layer, EntityType_Hero, &Position);

	Entity->HitPointMax = 3;
	Entity->CollisionBox = rect2{32, 32};

	//TODO: Set properties.

	return(*Entity);
}

//TODO: Rewrite, should operate on Level of a world
//om_internal entity
//AddWall(game_state *GameState, u32 PosX, u32 PosY)
//{
//	//TODO: The Position times the constant value is temporary. Remove this when entities got position representation
//	// within the world tiles.
//	world_position Position = {(r32)PosX * 70, (r32)PosY * 80}; // Get position based on tile x and y
//	entity Entity = *AddEntity(GameState, EntityType_Wall, &Position);
//
//	return (Entity);
//}

//TODO: Don't use the defined PIXELS_PER_TILE later.
om_internal b32
TestTile(r32 WallX, r32 WallY, entity *Target)
{
	b32 Hit = false;
	r32 X = Target->Position.x;
	r32 Y = Target->Position.y;
	WallX *= PIXELS_PER_TILE;
	WallY *= PIXELS_PER_TILE;

	if (X < WallX + PIXELS_PER_TILE &&
		X + PIXELS_PER_TILE > WallX &&
		Y < WallY + PIXELS_PER_TILE &&
		Y + PIXELS_PER_TILE > WallY)
	{
		Hit = true;
	}

	return (Hit);
}

om_internal void
MoveEntity(world_layer *Layer, entity *Entity, r32 DeltaTime, vector2 DeltaPosition)
{
	//TODO: Maybe add this into the entity itself?
	r32 EntitySpeed = 2.0f;
	DeltaPosition *= EntitySpeed;

	Entity->Position += DeltaPosition;

	for (int TileY = 0; TileY < 23; ++TileY)
	{
		for (int TileX = 0; TileX < 40; ++TileX)
		{
			world_tile Tile = Layer->Tiles[TileY * 40 + TileX];
			if (Tile.Collideable)
			{
				if (TestTile(TileX, TileY, Entity))
				{
					Entity->Position -= DeltaPosition;
				}
			}
		}
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
		char GrassBitmap[] = "C:\\Users\\Oskar\\Documents\\GitHub\\game\\Data\\groundTile.bmp";
		GameState->GrassBitmap = DEBUGLoadBitmap(GrassBitmap);

		char WaterBitmap[] = "C:\\Users\\Oskar\\Documents\\GitHub\\game\\Data\\waterTile.bmp";
		GameState->WaterBitmap = DEBUGLoadBitmap(WaterBitmap);

		char PlayerBitmap[] = "C:\\Users\\Oskar\\Documents\\GitHub\\game\\Data\\playerBitmap.bmp";
		GameState->PlayerBitmap = DEBUGLoadBitmap(PlayerBitmap);

		GameState->ToneHz = 256;

		// Initializing World
		world *World = nullptr;
		if (World == NULL)
		{
			// TODO: Temporary call. Don't use Virtual Alloc here and let Platform layer handle allocation.
			World = (world *)VirtualAlloc(0, sizeof(world), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
			GameState->World = World;
		}

		// TODO: Should later be loaded from level file.
		u32 WorldWidth = 40;
		u32 WorldHeight = 23;
		InitializeWorld(World, WorldWidth, WorldHeight);

		for (u32 TileY = 0; TileY < WorldHeight; ++TileY)
		{
			for (u32 TileX = 0; TileX < WorldWidth; ++TileX)
			{
				u32 TileValue = 1;

				if ((TileX == 0) || (TileY == 0) || (TileY == (WorldHeight -1)) || (TileX == (WorldWidth -1)))
				{
					TileValue = 2;

				}

				if (TileValue == 2)
				{
					World->Layers[0].Tiles[TileY * WorldWidth + TileX].Collideable = true;
				}

				World->Layers[0].Tiles[TileY * WorldWidth + TileX].Value = TileValue;
			}
		}

		for (u32 TileY = 0; TileY < WorldHeight; ++TileY)
		{
			for (u32 TileX = 0; TileX < WorldWidth; ++TileX)
			{
				u32 TileValue = 1;

				World->Layers[1].Tiles[TileY * WorldWidth + TileX].Value = TileValue;
			}
		}

		//TODO: This should be added by the level file later.
		world_layer *FirstLayer = &GameState->World->Layers[0];
		AddPlayer(FirstLayer, 1280 / 2, 720 / 2);
		GameState->ControlledEntity = &FirstLayer->Entities[0];

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
			entity* Player = GameState->ControlledEntity;
			vector2 DeltaPosition = {};
			if (Controller->MoveLeft.EndedDown)
			{
				//GameState->BlueOffset -= 1;
				//GameState->ControlledEntity->Position.x -= 1;
				DeltaPosition.x -= 1;
			}
			if (Controller->MoveRight.EndedDown)
			{
				//GameState->BlueOffset += 1;
				//GameState->ControlledEntity->Position.x += 1;
				DeltaPosition.x += 1;
			}
			if (Controller->MoveUp.EndedDown)
			{
				//GameState->ControlledEntity->Position.y -= 1;
				DeltaPosition.y -= 1;

			}
			if (Controller->MoveDown.EndedDown)
			{
				//GameState->ControlledEntity->Position.y += 1;
				DeltaPosition.y += 1;
			}

			MoveEntity(&GameState->World->Layers[0], Player, 0.0f, DeltaPosition);
		}
	}

	world *World = GameState->World;
	for (int LayerIndex = OM_ARRAYCOUNT(World->Layers) -1; LayerIndex >= 0; --LayerIndex) 
	{
		world_layer *Layer = &GameState->World->Layers[LayerIndex];
		for (u32 TileY = 0; TileY < GameState->World->WorldHeight; ++TileY)
		{
			for (u32 TileX = 0; TileX < GameState->World->WorldWidth; ++TileX)
			{
				world_tile Tile = Layer->Tiles[TileY * GameState->World->WorldWidth + TileX];
				if (Tile.Value == 1)
				{
					DrawBitmap(Buffer, &GameState->WaterBitmap, TileX * PIXELS_PER_TILE, TileY * PIXELS_PER_TILE, 0.0f);
				}
				if (Tile.Value == 2)
				{
					DrawBitmap(Buffer, &GameState->GrassBitmap, TileX * PIXELS_PER_TILE, TileY * PIXELS_PER_TILE, 0.0f);
				}
			}
		}
		for (int EntityIndex = 0; EntityIndex < Layer->EntityCount; ++EntityIndex)
		{
			entity Entity = Layer->Entities[EntityIndex];

			switch (Entity.Type)
			{
			case EntityType_Hero:
			{
				DrawBitmap(Buffer, &GameState->PlayerBitmap, Entity.Position.x, Entity.Position.y, 0.0f);

			} break;
			case EntityType_Monster:
			default:
				break;
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
}

om_internal void
GameGetSoundSamples(game_memory *Memory, game_sound_output_buffer *SoundBuffer)
{
	game_state *GameState = (game_state *)Memory->PermanentStorage;
	//GameOutputSound(SoundBuffer, GameState->ToneHz);
}