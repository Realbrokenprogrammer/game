#include "Game.h"
#include "Game_World.cpp"

#include <Windows.h> //TODO: This should later be removed.

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
	Entity->ID = { EntityIndex };
	Entity->Type = Type;
	Entity->Position = {(r32)Position->X, (r32)Position->Y};

	return (Entity);
}

//TODO: Should return reference or not?
om_internal entity *
AddPlayer(world_layer *Layer, u32 PositionX, u32 PositionY)
{
	world_position Position = { PositionX, PositionY, 0 };
	entity *Entity = AddEntity(Layer, EntityType_Hero, &Position);

	Entity->HitPointMax = 3;
	Entity->Collideable = true;
	Entity->CollisionBox = rect2{32, 32};
	Entity->Width = 32.0f;
	Entity->Height = 32.0f;
	//TODO: Set properties.

	return(Entity);
}

om_internal entity
AddGrass(world_layer *Layer, u32 PositionX, u32 PositionY)
{
	world_position Position{ PositionX, PositionY, 0 };
	entity *Entity = AddEntity(Layer, EntityType_GrassTile, &Position);

	Entity->Collideable = true;
	Entity->CollisionBox = rect2{ 32, 32 };
	Entity->Width = 32.0f;
	Entity->Height = 32.0f;

	return (*Entity);
}

om_internal entity
AddWater(world_layer *Layer, u32 PositionX, u32 PositionY)
{
	world_position Position{ PositionX, PositionY, 0 };
	entity *Entity = AddEntity(Layer, EntityType_WaterTile, &Position);

	Entity->Collideable = true;
	Entity->CollisionBox = rect2{ 32, 32 };

	return (*Entity);
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

	if (X < WallX + PIXELS_PER_TILE &&
		X + PIXELS_PER_TILE > WallX &&
		Y < WallY + PIXELS_PER_TILE &&
		Y + PIXELS_PER_TILE > WallY)
	{
		Hit = true;
	}

	return (Hit);
}

om_internal b32
ContainsPoint(entity *Entity, r32 PointX, r32 PointY)
{
	return (Entity->Position.x < PointX && Entity->Position.y < PointY &&
		Entity->Position.x + 32 > PointX &&
		Entity->Position.y + 32 > PointY);
}


om_internal b32
TestCollision(r32 WallX, r32 RelativeX, r32 RelativeY, r32 DeltaX, r32 DeltaY, r32 *tMin, r32 MinY, r32 MaxY)
{
	b32 Hit = false;

	r32 tEpsilon = 0.001f;
	if (DeltaX != 0.0f)
	{
		r32 tResult = (WallX - RelativeX) / DeltaX;
		r32 Y = RelativeY + tResult * DeltaY;
		if ((tResult >= 0.0f) && (*tMin > tResult))
		{
			if ((Y >= MinY) && (Y <= MaxY))
			{
				*tMin = OM_MAX(0.0f, tResult - tEpsilon);
				Hit = true;
			}
		}
	}

	return (Hit);
}

om_internal void
CanCollide(entity *A, entity *B)
{
	//TODO: Implement
}

//TODO: Collision with slopes / Non rectangle shapes.
om_internal void
MoveEntity(world_layer *Layer, entity *Entity, r32 DeltaTime, vector2 ddPosition)
{
	r32 ddLength = LengthSquared(ddPosition);
	if (ddLength > 1.0f)
	{
		ddPosition *= (1.0f / SquareRoot(ddLength));
	}

	//TODO: Maybe add this into the entity itself?
	r32 EntitySpeed = 50.0f; // m/s^2
	r32 Drag = 0.8f;

	ddPosition *= EntitySpeed;
	ddPosition += (-Drag * Entity->dPosition);

	//TODO: Write short comment with calculations
	vector2 OldPosition = Entity->Position;
	vector2 EntityDelta = (0.5f * ddPosition * Square(DeltaTime) + Entity->dPosition * DeltaTime);
	Entity->dPosition = ddPosition * DeltaTime + Entity->dPosition;
	
	vector2 NewPosition = OldPosition + EntityDelta;

	for (int Iteration = 0; Iteration < 4; ++Iteration) {
		r32 tMin = 1.0f;
		vector2 WallNormal = {};
		u32 HitEntityIndex = 0;

		vector2 DesiredPosition = Entity->Position + EntityDelta;

		if (Entity->Collideable) {
			for (u32 TestEntityIndex = 0; TestEntityIndex < Layer->EntityCount; ++TestEntityIndex)
			{
				if (TestEntityIndex != Entity->ID.Value)
				{
					entity *TestEntity = Layer->Entities + TestEntityIndex;
					if (TestEntity->Collideable)
					{
						r32 DiameterW = TestEntity->Width + Entity->Width;
						r32 DiameterH = TestEntity->Height + Entity->Height;

						vector2 MinCorner = -0.5f*Vector2(DiameterW, DiameterH);
						vector2 MaxCorner = 0.5f*Vector2(DiameterW, DiameterH);

						vector2 Rel = Entity->Position - TestEntity->Position;

						if (TestCollision(MinCorner.x, Rel.x, Rel.y, EntityDelta.x, EntityDelta.y, &tMin, MinCorner.y, MaxCorner.y))
						{
							WallNormal = vector2{ -1, 0 };
							HitEntityIndex = TestEntityIndex;
						}

						if (TestCollision(MaxCorner.x, Rel.x, Rel.y, EntityDelta.x, EntityDelta.y, &tMin, MinCorner.y, MaxCorner.y))
						{
							WallNormal = vector2{ 1, 0 };
							HitEntityIndex = TestEntityIndex;
						}

						if (TestCollision(MinCorner.y, Rel.y, Rel.x, EntityDelta.y, EntityDelta.x, &tMin, MinCorner.x, MaxCorner.x))
						{
							WallNormal = vector2{ 0, -1 };
							HitEntityIndex = TestEntityIndex;
						}

						if (TestCollision(MaxCorner.y, Rel.y, Rel.x, EntityDelta.y, EntityDelta.x, &tMin, MinCorner.x, MaxCorner.x))
						{
							WallNormal = vector2{ 0, 1 };
							HitEntityIndex = TestEntityIndex;
						}
					}
				}
			}
		}

		if (HitEntityIndex)
		{
			Entity->dPosition = Entity->dPosition - 1 * Inner(Entity->dPosition, WallNormal) * WallNormal;

			EntityDelta = DesiredPosition - Entity->Position;
			EntityDelta = EntityDelta - 1 * Inner(EntityDelta, WallNormal) * WallNormal;

			//TODO: Stairs etc.
		}

		// TODO: I moved the position update to after the Hit detection statement to make sure
		// that the entity wouldn't get stuck in the wall first and THEN the velocity would be corrected to
		// move the player along the wall. Tho this might be incorrect for the calculations. Check this up.
		Entity->Position += tMin * EntityDelta;
	}

	//TODO: Change using accel vector
	if ((Entity->dPosition.x == 0.0f) && (Entity->dPosition.y == 0.0f))
	{
		
	}
	else if (AbsoluteValue(Entity->dPosition.x) > AbsoluteValue(Entity->dPosition.y))
	{
		if (Entity->dPosition.x > 0)
		{
			// Change facing direction accordingly.
		}
		else
		{
			// Change facing direction accordingly.
		}
	}
	else
	{
		if (Entity->dPosition.y > 0)
		{
			// Change facing direction accordingly.
		}
		else
		{
			// Change facing direction accordingly.
		}
	}

	//TODO: Change the entity world position?
}


extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
	OM_ASSERT(sizeof(game_state) <= Memory->PermanentStorageSize);

	game_state *GameState = (game_state *)Memory->PermanentStorage;
	if (!Memory->IsInitialized)
	{
		char GrassBitmap[] = "C:\\Users\\Oskar\\Documents\\GitHub\\game\\Data\\groundTile.bmp";
		GameState->GrassBitmap = Memory->DEBUGLoadBitmap(GrassBitmap);

		char WaterBitmap[] = "C:\\Users\\Oskar\\Documents\\GitHub\\game\\Data\\waterTile.bmp";
		GameState->WaterBitmap = Memory->DEBUGLoadBitmap(WaterBitmap);

		char PlayerBitmap[] = "C:\\Users\\Oskar\\Documents\\GitHub\\game\\Data\\playerBitmap.bmp";
		GameState->PlayerBitmap = Memory->DEBUGLoadBitmap(PlayerBitmap);

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
				else if (TileX == (WorldWidth / 2) && (TileY+1 == (WorldHeight / 2)))
				{
					TileValue = 2;
				}

				if (TileValue == 2)
				{
					AddGrass(&World->Layers[0], TileX * PIXELS_PER_TILE, TileY * PIXELS_PER_TILE);
				}
				else
				{
				}
			}
		}

		for (u32 TileY = 0; TileY < WorldHeight; ++TileY)
		{
			for (u32 TileX = 0; TileX < WorldWidth; ++TileX)
			{
				u32 TileValue = 1;

				AddWater(&World->Layers[1], TileX * PIXELS_PER_TILE, TileY * PIXELS_PER_TILE);
			}
		}

		//TODO: This should be added by the level file later.
		world_layer *FirstLayer = &GameState->World->Layers[0];
		GameState->ControlledEntity = AddPlayer(FirstLayer, 1280 / 2, 720 / 2);

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
			vector2 ddPosition = {};
			if (Controller->MoveLeft.EndedDown)
			{
				//GameState->BlueOffset -= 1;
				//GameState->ControlledEntity->Position.x -= 1;
				ddPosition.x -= 1.0f;
			}
			if (Controller->MoveRight.EndedDown)
			{
				//GameState->BlueOffset += 1;
				//GameState->ControlledEntity->Position.x += 1;
				ddPosition.x += 1.0f;
			}
			if (Controller->MoveUp.EndedDown)
			{
				//GameState->ControlledEntity->Position.y -= 1;
				ddPosition.y -= 1.0f;
			}
			if (Controller->MoveDown.EndedDown)
			{
				//GameState->ControlledEntity->Position.y += 1;
				ddPosition.y += 1.0f;
			}

			//r32 PlayerSpeed = 50.0f; // Supposed to be (m/s)^2
			//ddPosition *= PlayerSpeed;

			////TODO: ODE
			//ddPosition += -0.2f*Player->dPosition;

			////TODO: Write short comment with my calculation
			//vector2 newPosition = Player->Position;
			//newPosition = 0.5f*ddPosition*Square(Input->dtForFrame) + Player->dPosition * Input->dtForFrame + newPosition;
			//Player->dPosition = ddPosition * Input->dtForFrame + Player->dPosition;

			//Player->Position = newPosition;

			MoveEntity(&GameState->World->Layers[0], Player, Input->dtForFrame, ddPosition);
		}
	}

	world *World = GameState->World;
	for (int LayerIndex = OM_ARRAYCOUNT(World->Layers) -1; LayerIndex >= 0; --LayerIndex) 
	{
		world_layer *Layer = &GameState->World->Layers[LayerIndex];
		for (int EntityIndex = 0; EntityIndex < Layer->EntityCount; ++EntityIndex)
		{
			entity Entity = Layer->Entities[EntityIndex];

			switch (Entity.Type)
			{
				case EntityType_Hero:
				{
					DrawBitmap(Buffer, &GameState->PlayerBitmap, Entity.Position.x, Entity.Position.y, 0.0f);
				} break;
				case EntityType_GrassTile:
				{
					DrawBitmap(Buffer, &GameState->GrassBitmap, Entity.Position.x, Entity.Position.y, 0.0f);
				} break;
				case EntityType_WaterTile:
				{
					DrawBitmap(Buffer, &GameState->WaterBitmap, Entity.Position.x, Entity.Position.y, 0.0f);
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


extern "C" GAME_GET_SOUND_SAMPLES(GameGetSoundSamples)
{
	game_state *GameState = (game_state *)Memory->PermanentStorage;
	//GameOutputSound(SoundBuffer, GameState->ToneHz);
}

BOOL WINAPI DllMain(
	_In_ HINSTANCE hinstDLL,
	_In_ DWORD     fdwReason,
	_In_ LPVOID    lpvReserved)
{
	return(TRUE);
}