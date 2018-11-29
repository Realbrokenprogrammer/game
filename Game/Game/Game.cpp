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
		
		char SlopeBitmap[] = "C:\\Users\\Oskar\\Documents\\GitHub\\game\\Data\\groundSlope_left.bmp";
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

		GameState->Time = 0;

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
	//TODO: Use PushClear to renderer instead of manually clearing.
	//Clear screen. 
	SoftwareDrawRect(Buffer, Vector2(0.0f, 0.0f), Vector2((r32)Buffer->Width, (r32)Buffer->Height), 0.0f, 0.0f, 0.0f);
#endif

	MoveCamera(&GameState->Camera, GameState->World, GameState->ControlledEntity->Position);

	GameState->Time += Input->dtForFrame;
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
		render_blueprint_header *Header = (render_blueprint_header *)(RenderBlueprint->PushBufferBase + BaseAddress);
		
		switch (Header->Type)
		{
			case RenderCommand_render_blueprint_clear:
			{
				render_blueprint_clear *Body = (render_blueprint_clear *)Header;

				// TODO: Clear

				BaseAddress += sizeof(*Body);
			} break;
			case RenderCommand_render_blueprint_line:
			{
				//TODO: Line
			} break;
			case RenderCommand_render_blueprint_circle:
			{
				//TODO: Circle
			} break;
			case RenderCommand_render_blueprint_triangle:
			{
				//TODO: Triangle
			} break;
			case RenderCommand_render_blueprint_rectangle:
			{
				render_blueprint_rectangle *Body = (render_blueprint_rectangle *)Header;
				vector2 Position = Body->Position - RenderBlueprint->Basis->Position;
				SoftwareDrawRect(Buffer, Position, Position + Body->Dimension, Body->R, Body->G, Body->B);
				BaseAddress += sizeof(*Body);
			} break;
			case RenderCommand_render_blueprint_bitmap:
			{
				render_blueprint_bitmap *Body = (render_blueprint_bitmap *)Header;
				vector2 Position = Body->Position - RenderBlueprint->Basis->Position;
#if 0
				DrawBitmap(Buffer, Body->Bitmap, Position, Body->A);
#else
				SoftwareDrawTransformedBitmap(Buffer, Position, 32.0f, 0.0f, Body->Bitmap, Vector4(1.0f, 1.0f, 1.0f, 1.0f));
#endif
				BaseAddress += sizeof(*Body);
			} break;

			InvalidDefaultCase;
		}
	}

	//DEBUGDrawRect(Buffer, Vector2(500.0f, 300.0f), 50.0f, GameState->Time, &GameState->PlayerBitmap, Vector4(1.0f, 1.0f, 1.0f, 1.0f));

	DestroyRenderBlueprint(RenderBlueprint);

	//TODO: Allow sample offsets here for more robust platform options
	/*RenderGradient(Buffer, GameState->BlueOffset, GameState->RedOffset);*/
}

extern "C" GAME_GET_SOUND_SAMPLES(GameGetSoundSamples)
{
	game_state *GameState = (game_state *)Memory->PermanentStorage;
	//GameOutputSound(SoundBuffer, GameState->ToneHz);
}