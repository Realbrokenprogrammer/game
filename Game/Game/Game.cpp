#include "Game.h"
#include "Game_World.cpp"
#include "Game_Physics.cpp"

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

//TODO: This needs serious rework.
om_internal void
DrawCircle(game_offscreen_buffer *Buffer, vector2 Center, r32 Radius, r32 R, r32 G, r32 B)
{
	u32 Color =
		((RoundReal32ToInt32(R * 255.0f) << 16) |
		(RoundReal32ToInt32(G * 255.0f) << 8) |
		(RoundReal32ToInt32(B * 255.0f)));

	for (r32 Angle = 0.0f; Angle < 360.0f; Angle++)
	{
		u32 X = Center.x - Radius * cosf(Angle);
		u32 Y = Center.y - Radius * sinf(Angle);
		u32 *Pixel = ((u32 *)Buffer->Memory + Buffer->Width * Y + X);
		*Pixel = Color;
	}
}

//TODO: Implement
om_internal void
DrawTriangle(game_offscreen_buffer *Buffer, triangle Triangle, r32 R, r32 G, r32 B)
{
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

inline vector2
GetCameraSpacePosition(game_state *GameState, entity *Entity)
{
	vector2 Result = Entity->Position - GameState->Camera.Position;
	
	return (Result);
}

inline vector2 
CenterCameraAtEntity(vector2 screenSize, entity *Entity)
{
	vector2 Result = Entity->Position - screenSize;

	return (Result);
}

om_internal void
UpdateCamera(game_state *GameState)
{
	//TODO: Use the movement blueprint from the entity that is being followed.
	r32 LerpVelocity = 0.025f;
	vector2 CameraCenter = GetCenter(GameState->Camera.CameraWindow);

	//TODO: Can this be cleaner?
	GameState->Camera.Position = Lerp(GameState->Camera.Position, (CenterCameraAtEntity(CameraCenter, GameState->ControlledEntity)), LerpVelocity);
	
	//TODO: Can this be cleaner?
	GameState->Camera.Position = Clamp(Vector2(0, 0), GameState->Camera.Position, 
		(Vector2(GameState->World->WorldWidth, GameState->World->WorldHeight) - GameState->Camera.CameraWindow.Max));
}

om_internal void 
SetCamera()
{

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

	entity_movement_blueprint MovementBlueprint = DefaultMovementBlueprint();
	MovementBlueprint.Speed = 50.0f;
	MovementBlueprint.Drag = 0.8f;

	Entity->MovementBlueprint = MovementBlueprint;
	Entity->HitPointMax = 3;
	Entity->Collideable = true;
	Entity->Width = 32.0f;
	Entity->Height = 32.0f;
	
	return(Entity);
}

om_internal entity
AddGrass(world_layer *Layer, u32 PositionX, u32 PositionY)
{
	world_position Position{ PositionX, PositionY, 0 };
	entity *Entity = AddEntity(Layer, EntityType_GrassTile, &Position);

	Entity->Collideable = true;
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

	ddPosition *= Entity->MovementBlueprint.Speed;
	ddPosition += (-Entity->MovementBlueprint.Drag * Entity->dPosition);

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
		GameState->RectPosX = 100;
		GameState->RectPosY = 100;

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
		InitializeWorld(World, WorldTileWidth*PIXELS_PER_TILE, WorldTileHeight*PIXELS_PER_TILE);

		for (u32 TileY = 0; TileY < WorldTileHeight; ++TileY)
		{
			for (u32 TileX = 0; TileX < WorldTileWidth; ++TileX)
			{
				u32 TileValue = 1;

				if ((TileX == 0) || (TileY == 0) || (TileY == (WorldTileHeight -1)) || (TileX == (WorldTileWidth -1)))
				{
					TileValue = 2;

				}
				else if (TileX == (WorldTileWidth / 2) && (TileY+1 == (WorldTileHeight / 2)))
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

		for (u32 TileY = 0; TileY < WorldTileHeight; ++TileY)
		{
			for (u32 TileX = 0; TileX < WorldTileWidth; ++TileX)
			{
				u32 TileValue = 1;

				AddWater(&World->Layers[1], TileX * PIXELS_PER_TILE, TileY * PIXELS_PER_TILE);
			}
		}

		//TODO: This should be added by the level file later.
		world_layer *FirstLayer = &GameState->World->Layers[0];
		GameState->ControlledEntity = AddPlayer(FirstLayer, 1280 / 2, 720 / 2);

		r32 ScreenCenterX = 0.5f * (r32)Buffer->Width;
		r32 ScreenCenterY = 0.5f * (r32)Buffer->Height;

		GameState->Camera = {};
		GameState->Camera.CameraWindow = { {0, 0}, {(r32)Buffer->Width, (r32)Buffer->Height} };

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
				//ddPosition.x -= 1.0f;
				GameState->RectPosX -= 1.0f;
			}
			if (Controller->MoveRight.EndedDown)
			{
				//ddPosition.x += 1.0f;
				GameState->RectPosX += 1.0f;
			}
			if (Controller->MoveUp.EndedDown)
			{
				//ddPosition.y -= 1.0f;
				GameState->RectPosY -= 1.0f;
			}
			if (Controller->MoveDown.EndedDown)
			{
				//ddPosition.y += 1.0f;
				GameState->RectPosY += 1.0f;
			}

			//MoveEntity(&GameState->World->Layers[0], Player, Input->dtForFrame, ddPosition);
		}
	}

#if 1
	//Clear screen.
	DrawRect(Buffer, Vector2(0.0f, 0.0f), Vector2((r32)Buffer->Width, (r32)Buffer->Height), 0.0f, 0.0f, 0.0f);
#endif
	//UpdateCamera(GameState);

	vector2 Center = GameState->Camera.CameraWindow.Max;
	Center.x = 300.0f;
	Center.y = 300.0f;
	r32 Radius = 20;

	shape CenterBall = {};
	CenterBall.CollisionShape = CollisionShape_Circle;
	CenterBall.Circle = {Radius, Center};

	shape CenterRect = {};
	CenterRect.CollisionShape = CollisionShape_Rectangle;
	CenterRect.Rectangle = { {150.0f, 150.0f}, {150.0f + 32.0f, 150.0f + 32.0f} };

	shape CenterTriangle = {};
	CenterTriangle.CollisionShape = CollisionShape_Triangle;
	CenterTriangle.Triangle = { {100.0f, 100.0f}, {200.0f, 100.0f}, {150.0f, 200.0f} };

	shape PlayerBall = {};
	PlayerBall.CollisionShape = CollisionShape_Circle;
	PlayerBall.Circle = { Radius, {GameState->RectPosX+16, GameState->RectPosY+16} };

	shape PlayerRect = {};
	PlayerRect.CollisionShape = CollisionShape_Rectangle;
	PlayerRect.Rectangle = { {GameState->RectPosX, GameState->RectPosY}, {GameState->RectPosX + 32.0f, GameState->RectPosY + 32.0f} };

	b32 INTERSECT = Test(PlayerRect, CenterRect) || Test(PlayerRect, CenterTriangle) || Test(CenterBall, PlayerRect);
	/*world *World = GameState->World;
	for (int LayerIndex = OM_ARRAYCOUNT(World->Layers) -1; LayerIndex >= 0; --LayerIndex) 
	{
		world_layer *Layer = &GameState->World->Layers[LayerIndex];
		for (int EntityIndex = 0; EntityIndex < Layer->EntityCount; ++EntityIndex)
		{
			entity *Entity = Layer->Entities + EntityIndex;

			switch (Entity->Type)
			{
				case EntityType_Hero:
				{
					DrawBitmap(Buffer, &GameState->PlayerBitmap, GetCameraSpacePosition(GameState, Entity), 0.0f);
				} break;
				case EntityType_GrassTile:
				{
					DrawBitmap(Buffer, &GameState->GrassBitmap, GetCameraSpacePosition(GameState, Entity), 0.0f);
				} break;
				case EntityType_WaterTile:
				{
					DrawBitmap(Buffer, &GameState->WaterBitmap, GetCameraSpacePosition(GameState, Entity), 0.0f);
				} break;
				case EntityType_Monster:
				default:
					break;
			}
		}
	}*/

	vector3 color = {1.0f, 0.0f, 0.0f};
	if (INTERSECT)
	{
		color = { 0.0f, 1.0f, 0.0f };
	}

	//DrawCircle(Buffer, PlayerBall.Circle.Centre, Radius, 1.0f, 0.0f, 0.0f);
	
	DrawRect(Buffer, PlayerRect.Rectangle.Min, PlayerRect.Rectangle.Max, 0.0f, 0.0f, 1.0f);
	DrawCircle(Buffer, Center, Radius, color.r, color.g, color.b);
	DrawRect(Buffer, CenterRect.Rectangle.Min, CenterRect.Rectangle.Max, color.r, color.g, color.b);
	Memory->DEBUGDrawTriangle(CenterTriangle.Triangle.p1, CenterTriangle.Triangle.p2, CenterTriangle.Triangle.p3, color);

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