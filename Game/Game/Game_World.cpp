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
InitializeWorld(world *World, u32 WorldWidth, u32 WorldHeight)
{
	World->WorldWidth = WorldWidth;
	World->WorldHeight = WorldHeight;
}