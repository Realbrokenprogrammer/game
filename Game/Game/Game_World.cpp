#define PIXELS_PER_TILE 16

inline world_position
NullPosition(void)
{
	world_position Result = {};

	return (Result);
}

inline b32
IsValid(world_position Position)
{
	return false;
}


inline world_difference
SubtractPosition2D(world *World, world_position *A, world_position *B)
{
	world_difference Result;

	vector2 deltaXY = { (r32)A->X - (r32)B->X,
						(r32)A->Y - (r32)B->Y };

	Result.deltaXY = World->TileSizeInMeters * deltaXY;

	return (Result);
}

inline void
ChangeEntityLocation(world *World, u32 EntityIndex, entity *Entity, world_position *OldPosition, world_position *NewPosition)
{
	//TODO: This is not bulletproof.

	if (NewPosition)
	{
		Entity->Position = {(r32)NewPosition->X, (r32)NewPosition->Y};
	}
	else
	{
		world_position P = NullPosition();
		Entity->Position = {(r32)P.X, (r32)P.Y};
	}
}

om_internal void
InitializeWorld(world *World, r32 TileSizeInMeters)
{
	World->TileSizeInMeters = TileSizeInMeters;
	World->PixelsPerTileInMeters = (r32)PIXELS_PER_TILE * TileSizeInMeters;

	for (u32 TileIndex = 0; TileIndex < OM_ARRAYCOUNT(World->Tiles); ++TileIndex)
	{
		World->Tiles[TileIndex].X = INT32_MAX;

	}
}