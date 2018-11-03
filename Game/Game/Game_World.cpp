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