#ifndef GAME_ENTITY_H
#define GAME_ENTITY_H
#pragma once

struct entity_id
{
	u32 Value;
};

enum entity_type
{
	EntityType_Null,

	EntityType_Hero,
	EntityType_Monster,
	EntityType_Wall
};

enum entity_flags
{
	EntityFlag_Collides,
	EntityFlag_Deleted,
	EntityFlag_Active
};

enum entity_movement
{
	//TODO: Figure out names for these.
};

struct hit_point
{
	u8 Flags;
	u8 Amount;
};

struct entity
{
	entity_id ID;

	//TODO: Camera positions

	entity_type Type;
	u32 Flags;

	vector2 Position;
	
	rect2 CollisionBox;

	u32 HitPointMax;
	hit_point HitPoint;

	entity_movement Movement;
};

#endif // GAME_ENTITY_H
