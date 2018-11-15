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

	for (u32 i = 0; i < World->Columns * World->Rows * 2; ++i)
	{
		World->Buckets.push_back(std::vector<entity *>());
	}
}

om_internal void
ClearWorldBuckets(world *World)
{
	for (u32 i = 0; i < World->Columns * World->Rows * 2; ++i)
	{
		World->Buckets[i].clear();
	}
}

om_internal void
AddBucket(vector2 Vector, r32 Width, u32 CellSize, std::vector<u32> &Bucket)
{
	u32 Cell = (u32)(Vector.x / CellSize) + (u32)(Vector.y / CellSize) * Width;

	if ((std::count(Bucket.begin(), Bucket.end(), Cell)))
	{
	}
	else
	{
		Bucket.push_back(Cell);
	}
}

om_internal std::vector<u32>
GetIDForEntity(world *World, entity *Entity)
{
	std::vector<u32> Result;

	vector2 Min = Entity->Position;
	vector2 Max = { Entity->Position.x + 32.0f, Entity->Position.y + 32.0f };
	r32 Width = World->WorldWidth / World->CellSize;

	// Top Left
	AddBucket(Min, Width, World->CellSize, Result);
	// Top Right
	AddBucket({Max.x, Min.y}, Width, World->CellSize, Result);
	// Bottom Right
	AddBucket({Max.x, Max.y}, Width, World->CellSize, Result);
	// BottomLeft
	AddBucket({Min.x, Max.y}, Width, World->CellSize, Result);

	return (Result);
}

om_internal void
RegisterEntity(world *World, entity *Entity)
{
	std::vector<u32> CellIds = GetIDForEntity(World, Entity);
	for (u32 Index : CellIds)
	{
		World->Buckets[Index].push_back(Entity);
	}
}

om_internal std::vector<entity *>
GetNearbyEntities(world *World, entity *Entity)
{
	std::vector<entity *> Result;
	std::vector<u32> CellIds = GetIDForEntity(World, Entity);

	for (u32 Index : CellIds)
	{
		Result.insert(std::end(Result), std::begin(World->Buckets[Index]), std::end(World->Buckets[Index]));
	}

	return (Result);
}