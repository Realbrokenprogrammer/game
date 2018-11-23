#include "Game.h"
#include "Game_World.cpp"
#include "Game_Physics.cpp"
#include "Game_Camera.cpp"
#include "Game_Renderer.cpp"

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
DrawCircle(game_offscreen_buffer *Buffer, vector2 Center, r32 Radius, r32 R, r32 G, r32 B)
{
	u32 Color =
		((RoundReal32ToInt32(R * 255.0f) << 16) |
		(RoundReal32ToInt32(G * 255.0f) << 8) |
		(RoundReal32ToInt32(B * 255.0f)));

	for (r32 Angle = 0.0f; Angle < 360.0f; Angle++)
	{
		i32 X = (i32)(Center.x - Radius * cosf(Angle));
		i32 Y = (i32)(Center.y - Radius * sinf(Angle));

		if (X < 0)
		{
			X = 0;
		}

		if (Y < 0)
		{
			Y = 0;
		}

		if (Y > Buffer->Height)
		{
			Y = Buffer->Height;
		}

		if (X > Buffer->Width)
		{
			X = Buffer->Width;
		}

		u32 *Pixel = ((u32 *)Buffer->Memory + Buffer->Width * Y + X);
		*Pixel = Color;
	}
}

om_internal void
DrawLine(game_offscreen_buffer *Buffer, vector2 Start, vector2 End, r32 R, r32 G, r32 B)
{
	if (Start.x > End.x)
	{
		vector2 Temp = Start;
		Start = End;
		End = Temp;
	}

	if (Start.x < 0)
	{
		Start.x = 0;
	}

	if (Start.y < 0)
	{
		Start.y = 0;
	}

	if (End.x > Buffer->Width)
	{
		End.x = (r32)Buffer->Width;
	}

	if (End.y > Buffer->Height)
	{
		End.y = (r32)Buffer->Height;
	}

	r32 Coefficient = 0.0f;
	if (End.x - Start.x == 0)
	{
		Coefficient = 10000;
		if (End.y - Start.y < 0)
		{
			Coefficient = Coefficient * -1;
		}
	}
	else
	{
		Coefficient = (End.y - Start.y) / (End.x - Start.x);
	}

	r32 CX;
	r32 CY;

	if (Coefficient < 0)
	{
		CX = 1 / (-1 + Coefficient) * -1;
		CY = Coefficient / (-1 + Coefficient) * -1;
	}
	else
	{
		CX = 1 / (1 + Coefficient);
		CY = Coefficient / (1 + Coefficient);
	}

	CX *= 1.00001f;
	CY *= 1.00001f;

	r32 ToLength = SquareRoot((End.x - Start.x) * (End.x - Start.x) + (End.y - Start.y) * (End.y - Start.y));
	r32 Length = 0;
	r32 Increment = SquareRoot(CX * CX + CY * CY);

	u32 Color =
		((RoundReal32ToInt32(R * 255.0f) << 16) |
		(RoundReal32ToInt32(G * 255.0f) << 8) |
		(RoundReal32ToInt32(B * 255.0f)));

	for (int Index = 0; Length < ToLength + 0.5f; ++Index)
	{
		r32 X = Start.x + CX * Index;
		r32 Y = Start.y + CY * Index;

		u32 *Pixel = ((u32 *)Buffer->Memory + Buffer->Width * (u32)Y + (u32)X);
		*Pixel = Color;

		Length += Increment;
	}
}

om_internal void
DrawTriangle(game_offscreen_buffer *Buffer, triangle Triangle, r32 R, r32 G, r32 B)
{
	DrawLine(Buffer, Triangle.p1, Triangle.p2, R, G, B);
	DrawLine(Buffer, Triangle.p2, Triangle.p3, R, G, B);
	DrawLine(Buffer, Triangle.p3, Triangle.p1, R, G, B);
}

om_internal void
DrawBitmap(game_offscreen_buffer *Buffer, loaded_bitmap *Bitmap, vector2 Target, r32 ColorAlpha)
{
	i32 MinX = RoundReal32ToInt32(Target.x);
	i32 MinY = RoundReal32ToInt32(Target.y);
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

	u32 *SourceRow = (Bitmap->Pixels + SourceOffsetX) + (SourceOffsetY * Bitmap->Width);
	u8 *DestinationRow = ((u8 *)Buffer->Memory + MinX * Buffer->BytesPerPixel + MinY * Buffer->Pitch);
	for (int Y = MinY; Y < MaxY; ++Y)
	{
		u32 *Destination = (u32 *)DestinationRow;
		u32 *Source = SourceRow;
		for (int X = MinX; X < MaxX; ++X)
		{
			*Destination++ = *Source++;
		}

		DestinationRow += Buffer->Pitch;
		SourceRow += Bitmap->Width;
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

inline entity_movement_blueprint
DefaultMovementBlueprint(void)
{
	entity_movement_blueprint Result;

	Result.Speed = 1.0f;
	Result.Drag = 0.0f;

	return (Result);
}

//TODO: Perhaps don't return pointer to the entity?
om_internal entity *
AddEntity(world_layer *Layer, entity_type Type, vector2 Position)
{
	OM_ASSERT(Layer->EntityCount < MAX_ENTITIES);
	u32 EntityIndex = Layer->EntityCount++;

	entity *Entity = Layer->Entities + EntityIndex;
	*Entity = {};
	Entity->ID = { EntityIndex };
	Entity->Type = Type;
	Entity->Position = Position;

	return (Entity);
}

//TODO: Should return reference or not?
om_internal entity *
AddPlayer(world_layer *Layer, vector2 Position)
{
	entity *Entity = AddEntity(Layer, EntityType_Hero, Position);

	entity_physics_blueprint PhysicsBlueprint = {};
	PhysicsBlueprint.CollisionShape = CollisionShape_Rectangle;
	PhysicsBlueprint.Rectangle = { Entity->Position, {Entity->Position.x + 32.0f, Entity->Position.y + 32.0f} };

	entity_movement_blueprint MovementBlueprint = DefaultMovementBlueprint();
	MovementBlueprint.Speed = 50.0f;
	MovementBlueprint.Drag = 0.8f;

	Entity->MovementBlueprint = MovementBlueprint;
	Entity->HitPointMax = 3;
	Entity->Collideable = true;
	Entity->PhysicsBlueprint = PhysicsBlueprint;
	Entity->Width = 32.0f;
	Entity->Height = 32.0f;
	
	return(Entity);
}

om_internal entity
AddGrass(world_layer *Layer, vector2 Position)
{
	entity *Entity = AddEntity(Layer, EntityType_GrassTile, Position);

	entity_physics_blueprint PhysicsBlueprint = {};
	PhysicsBlueprint.CollisionShape = CollisionShape_Rectangle;
	PhysicsBlueprint.Rectangle = { Entity->Position, {Entity->Position.x + 32.0f, Entity->Position.y + 32.0f} };

	Entity->Collideable = true;
	Entity->PhysicsBlueprint = PhysicsBlueprint;
	Entity->Width = 32.0f;
	Entity->Height = 32.0f;

	return (*Entity);
}

om_internal entity
AddWater(world_layer *Layer, vector2 Position)
{
	entity *Entity = AddEntity(Layer, EntityType_WaterTile, Position);

	entity_physics_blueprint PhysicsBlueprint = {};
	PhysicsBlueprint.CollisionShape = CollisionShape_Rectangle;
	PhysicsBlueprint.Rectangle = { Entity->Position, {Entity->Position.x + 32.0f, Entity->Position.y + 32.0f} };

	Entity->Collideable = true;
	Entity->PhysicsBlueprint = PhysicsBlueprint;

	return (*Entity);
}

om_internal entity
AddSlope(world_layer *Layer, vector2 Position)
{
	entity *Entity = AddEntity(Layer, EntityType_SlopeTile, Position);

	entity_physics_blueprint PhysicsBlueprint = {};
	PhysicsBlueprint.CollisionShape = CollisionShape_Triangle;
	vector2 Point1 = { 32.0f, 0.0f };
	vector2 Point2 = { 0.0f, 32.0f };
	vector2 Point3 = { 32.0f, 32.0f };

	Point1 += Position;
	Point2 += Position;
	Point3 += Position;
	
	PhysicsBlueprint.Triangle = {Point1, Point2, Point3};

	Entity->Collideable = true;
	Entity->PhysicsBlueprint = PhysicsBlueprint;

	return (*Entity);
}

//TODO: This is not used but kept here for reference.
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
MoveEntity(world *World, entity *Entity, r32 DeltaTime, vector2 ddPosition)
{
	//TODO: This should be refactored, here we can't move entities outside of layer 1
	world_layer *Layer = &World->Layers[0];

	r32 ddLength = LengthSquared(ddPosition);
	if (ddLength > 1.0f)
	{
		ddPosition *= (1.0f / SquareRoot(ddLength));
	}

	ddPosition *= Entity->MovementBlueprint.Speed;
	ddPosition += (-Entity->MovementBlueprint.Drag * Entity->dPosition);

	//TODO: Write short comment with calculations
	vector2 OldPosition = Entity->Position;
	vector2 EntityDelta = (0.5f * ddPosition * Square(DeltaTime) + Entity->dPosition * DeltaTime);
	Entity->dPosition = ddPosition * DeltaTime + Entity->dPosition;
	
	vector2 NewPosition = OldPosition + EntityDelta;

	for (int Iteration = 0; Iteration < 4; ++Iteration) 
	{
		r32 tMin = 1.0f;
		vector2 WallNormal = {};
		b32 HitEntity = false;
		r32 PenetrationDepth = 0.0f;

		vector2 DesiredPosition = Entity->Position + EntityDelta;
		
		entity ** TestEntities = GetNearbyEntities(World, Entity);
		u32 TestEntitiesCount = OM_ARRAY_COUNT(TestEntities);

		if (Entity->Collideable) {

			for (u32 TestEntityIndex = 0; TestEntityIndex < TestEntitiesCount; ++TestEntityIndex)
			{
				entity *TestEntity = TestEntities[TestEntityIndex];
				if (TestEntity->ID.Value != Entity->ID.Value && TestEntity->Collideable)
				{
					// TODO: Later we might want to add the Entity's Position derivations into the physics spec
					// so we don't have to update this rect here all the time.
					Entity->PhysicsBlueprint.Rectangle = { {DesiredPosition}, {DesiredPosition.x + 32.0f, DesiredPosition.y + 32.0f} };
					
					collision_info CollisionInfo = TestCollision(Entity->PhysicsBlueprint, TestEntity->PhysicsBlueprint);
					if (CollisionInfo.IsColliding)
					{
						WallNormal = CollisionInfo.PenetrationNormal;
						PenetrationDepth = CollisionInfo.PenetrationDepth;
						HitEntity = true;
					}

					// TODO: Review Collision resolution.
					if (HitEntity)
					{
						Entity->dPosition = Entity->dPosition - 1.0f * Inner(Entity->dPosition, WallNormal) * WallNormal;

						EntityDelta = DesiredPosition - Entity->Position;
						EntityDelta = EntityDelta - 1 * Inner(EntityDelta, WallNormal) * WallNormal;
						DesiredPosition = Entity->Position + EntityDelta;

						//TODO: Stairs etc.
					}
				}
			}
		}

		OM_ARRAY_FREE(TestEntities);

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
		
		char SlopeBitmap[] = "C:\\Users\\Oskar\\Documents\\GitHub\\game\\Data\\groundSlope.bmp";
		GameState->SlopeBitmap = Memory->DEBUGLoadBitmap(SlopeBitmap);

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
		u32 WorldTileWidth = 60;
		u32 WorldTileHeight = 23;
		u32 WorldCellSize = 128; //TODO: Set more educated value for this.
		InitializeWorld(World, WorldTileWidth*PIXELS_PER_TILE, WorldTileHeight*PIXELS_PER_TILE, WorldCellSize);

		for (u32 TileY = 0; TileY < WorldTileHeight; ++TileY)
		{
			for (u32 TileX = 0; TileX < WorldTileWidth; ++TileX)
			{
				u32 TileValue = 1;
				b32 IsMiddle = 0;

				if ((TileX == 0) || (TileY == 0) || (TileY == (WorldTileHeight -1)) || (TileX == (WorldTileWidth -1)))
				{
					TileValue = 2;

				}
				else if (TileX == (WorldTileWidth / 2) && (TileY+1 == (WorldTileHeight / 2)))
				{
					TileValue = 2;
					IsMiddle = 1;
				}
				else if (TileX == (WorldTileWidth / 2) && (TileY == (WorldTileHeight-2)))
				{
					TileValue = 3;
				}
				else if (TileX == ((WorldTileWidth / 2)+1) && (TileY == (WorldTileHeight - 2)))
				{
					TileValue = 2;
				}

				if (TileValue == 2)
				{
					if (IsMiddle)
					{
						GameState->Player2 = AddPlayer(&World->Layers[0], { (r32)(TileX * PIXELS_PER_TILE), (r32)(TileY * PIXELS_PER_TILE) });
					}
					else
					{
						AddGrass(&World->Layers[0], { (r32)(TileX * PIXELS_PER_TILE), (r32)(TileY * PIXELS_PER_TILE) });
					}
				}
				else if (TileValue == 3)
				{
					AddSlope(&World->Layers[0], { (r32)(TileX * PIXELS_PER_TILE), (r32)(TileY * PIXELS_PER_TILE) });
				}
				else
				{
				}
			}
		}

		for (u32 TileY = 0; TileY < WorldTileHeight; ++TileY)
		{
			for (u32 TileX = 0; TileX < WorldTileWidth; ++TileX)
			{
				u32 TileValue = 1;

				AddWater(&World->Layers[1], { (r32)(TileX * PIXELS_PER_TILE), (r32)(TileY * PIXELS_PER_TILE) });
			}
		}

		//TODO: This should be added by the level file later.
		world_layer *FirstLayer = &GameState->World->Layers[0];
		GameState->ControlledEntity = AddPlayer(FirstLayer, { 1280.0f / 2.0f, 720.0f / 2.0f });
		GameState->Player = GameState->ControlledEntity;

		r32 ScreenCenterX = 0.5f * (r32)Buffer->Width;
		r32 ScreenCenterY = 0.5f * (r32)Buffer->Height;

		GameState->Camera = {};
		GameState->Camera.CameraWindow = { {0, 0}, {(r32)Buffer->Width, (r32)Buffer->Height} };

		//TODO: This may be more appropriate to let the platform layer do.
		Memory->IsInitialized = true;
	}

	//TODO: Can we make this more efficient?
	//TODO: Update Bucket entries instead of clearing and adding all entities every frame since most are static entities.
	ClearWorldBuckets(GameState->World);
	world_layer *FirstLayer = &GameState->World->Layers[0];
	for (u32 EntityIndex = 0; EntityIndex < FirstLayer->EntityCount; ++EntityIndex)
	{
		entity *Entity = FirstLayer->Entities + EntityIndex;
		RegisterEntity(GameState->World, Entity);
	}

	for (int ControllerIndex = 0; ControllerIndex < OM_ARRAYCOUNT(Input->Controllers); ++ControllerIndex)
	{
		game_controller_input *Controller = GetController(Input, ControllerIndex);
		
		if (Controller->IsAnalog)
		{
			GameState->ToneHz = 256 + (int)(128.0f*(Controller->StickAverageY));
		}
		else
		{
			entity* Player = GameState->ControlledEntity;
			vector2 ddPosition = {};
			if (Controller->MoveLeft.EndedDown)
			{
				ddPosition.x -= 1.0f;
			}
			if (Controller->MoveRight.EndedDown)
			{
				ddPosition.x += 1.0f;
			}
			if (Controller->MoveUp.EndedDown)
			{
				ddPosition.y -= 1.0f;
			}
			if (Controller->MoveDown.EndedDown)
			{
				ddPosition.y += 1.0f;
			}
			if (Controller->LeftShoulder.EndedDown)
			{
				GameState->ControlledEntity = GameState->Player;
			}
			if (Controller->RightShoulder.EndedDown)
			{
				GameState->ControlledEntity = GameState->Player2;
			}

			MoveEntity(GameState->World, Player, Input->dtForFrame, ddPosition);
		}
	}

#if 1
	//Clear screen.
	DrawRect(Buffer, Vector2(0.0f, 0.0f), Vector2((r32)Buffer->Width, (r32)Buffer->Height), 0.0f, 0.0f, 0.0f);
#endif

	MoveCamera(&GameState->Camera, GameState->World, GameState->ControlledEntity->Position);

	render_basis Basis = { GameState->Camera.Position };
	render_blueprint *RenderBlueprint = CreateRenderBlueprint(&Basis, om_megabytes(4));

	world *World = GameState->World;
	for (int LayerIndex = OM_ARRAYCOUNT(World->Layers) -1; LayerIndex >= 0; --LayerIndex) 
	{
		world_layer *Layer = &GameState->World->Layers[LayerIndex];
		for (u32 EntityIndex = 0; EntityIndex < Layer->EntityCount; ++EntityIndex)
		{
			entity *Entity = Layer->Entities + EntityIndex;

			switch (Entity->Type)
			{
				case EntityType_Hero:
				{
					PushBitmap(RenderBlueprint, &GameState->PlayerBitmap, Entity->Position, Vector2(0.0f, 0.0f));
				} break;
				case EntityType_GrassTile:
				{
					PushBitmap(RenderBlueprint, &GameState->GrassBitmap, Entity->Position, Vector2(0.0f, 0.0f));
				} break;
				case EntityType_WaterTile:
				{
					PushBitmap(RenderBlueprint, &GameState->WaterBitmap, Entity->Position, Vector2(0.0f, 0.0f));
				} break;
				case EntityType_SlopeTile:
				{
					PushBitmap(RenderBlueprint, &GameState->SlopeBitmap, Entity->Position, Vector2(0.0f, 0.0f));
				} break;
				case EntityType_Monster:
				default:
					break;
			}
		}
	}

	for (u32 BaseAddress = 0; BaseAddress < RenderBlueprint->PushBufferSize;)
	{
		render_entry_bitmap *RenderEntry = (render_entry_bitmap *)(RenderBlueprint->PushBufferBase + BaseAddress);
		BaseAddress += sizeof(render_entry_bitmap);

		vector2 Position = RenderEntry->Position - RenderBlueprint->Basis->Position;
		if (RenderEntry->Bitmap)
		{
			DrawBitmap(Buffer, RenderEntry->Bitmap, Position, RenderEntry->A);
		}
	}

	DestroyRenderBlueprint(RenderBlueprint);

	//TODO: Allow sample offsets here for more robust platform options
	/*RenderGradient(Buffer, GameState->BlueOffset, GameState->RedOffset);*/
}

extern "C" GAME_GET_SOUND_SAMPLES(GameGetSoundSamples)
{
	game_state *GameState = (game_state *)Memory->PermanentStorage;
	//GameOutputSound(SoundBuffer, GameState->ToneHz);
}