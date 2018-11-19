#define PIXELS_PER_TILE 32

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

om_internal void
InitializeWorld(world *World, u32 WorldWidth, u32 WorldHeight, u32 WorldCellSize)
{
	World->WorldWidth = WorldWidth;
	World->WorldHeight = WorldHeight;
	World->CellSize = WorldCellSize;
	World->Columns = World->WorldWidth / World->CellSize;
	World->Rows = (World->WorldHeight / World->CellSize) + 1;

	for (u32 BucketIndex = 0; BucketIndex < World->Columns * World->Rows * 2; ++BucketIndex)
	{
		entity **Bucket = NULL;
		OM_ARRAY_PUSH(World->Buckets, Bucket);
	}
}

om_internal void
ClearWorldBuckets(world *World)
{
	for (u32 BucketIndex = 0; BucketIndex < World->Columns * World->Rows * 2; ++BucketIndex)
	{
		OM_ARRAY_FREE(World->Buckets[BucketIndex]);
	}
	OM_ARRAY_FREE(World->Buckets);

	World->Buckets = NULL;
	for (u32 BucketIndex = 0; BucketIndex < World->Columns * World->Rows * 2; ++BucketIndex)
	{
		entity **Bucket = NULL;
		OM_ARRAY_PUSH(World->Buckets, Bucket);
	}
}

om_internal u32 *
AddBucket(vector2 Vector, r32 Width, r32 CellSize, u32 *Bucket)
{
	u32 Cell = (u32)(Vector.x / CellSize) + (u32)(Vector.y / CellSize) * Width;
	u32 BucketSize = OM_ARRAY_COUNT(Bucket);

	for (u32 BucketIndex = 0; BucketIndex < BucketSize; ++BucketIndex)
	{
		if (Bucket[BucketIndex] == Cell)
		{
			return (Bucket);
		}
	}

	OM_ARRAY_PUSH(Bucket, Cell);
	return (Bucket);
}

om_internal u32 *
GetIDForEntity(world *World, entity *Entity)
{
	u32 *Result = NULL;

	vector2 Min = Entity->Position;
	vector2 Max = { Entity->Position.x + 32.0f, Entity->Position.y + 32.0f };
	r32 Width = World->WorldWidth / World->CellSize;

	// Top Left
	Result = AddBucket(Min, Width, World->CellSize, Result);
	// Top Right
	Result = AddBucket({ Max.x, Min.y }, Width, World->CellSize, Result);
	// Bottom Right
	Result = AddBucket({ Max.x, Max.y }, Width, World->CellSize, Result);
	// BottomLeft
	Result = AddBucket({ Min.x, Max.y }, Width, World->CellSize, Result);

	return (Result);
}

om_internal void
RegisterEntity(world *World, entity *Entity)
{
	u32 *CellIds = GetIDForEntity(World, Entity);
	u32 CellIdsSize = OM_ARRAY_COUNT(CellIds);

	for (u32 CellIndex = 0; CellIndex < CellIdsSize; ++CellIndex)
	{
		u32 Cell = CellIds[CellIndex];
		OM_ARRAY_PUSH(World->Buckets[Cell], Entity);
	}

	OM_ARRAY_FREE(CellIds);
}

om_internal entity **
GetNearbyEntities(world *World, entity *Entity)
{
	entity **Result = NULL;
	u32 *CellIds = GetIDForEntity(World, Entity);
	u32 CellIdsSize = OM_ARRAY_COUNT(CellIds);


	for (u32 CellIndex = 0; CellIndex < CellIdsSize; ++CellIndex)
	{
		u32 Cell = CellIds[CellIndex];
		u32 BucketSize = OM_ARRAY_COUNT(World->Buckets[Cell]);
		for (u32 BucketIndex = 0; BucketIndex < BucketSize; ++BucketIndex)
		{
			OM_ARRAY_PUSH(Result, World->Buckets[Cell][BucketIndex]);
		}
	}

	OM_ARRAY_FREE(CellIds);

	return (Result);
}