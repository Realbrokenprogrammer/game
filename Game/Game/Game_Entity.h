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
	EntityType_GrassTile,
	EntityType_WaterTile,
	EntityType_SlopeTileLeft,
	EntityType_SlopeTileRight
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

struct entity_movement_blueprint
{
	r32 Speed;
	r32 Drag;
};

struct entity_physics_blueprint
{
	b32 Solid; //TODO: Make use of this later.

	transform Transform;
	collision_shape CollisionShape;
	//NOTE: This is union currently not being used as intended. The rectangle points are calculated
	// in both the renderer and the physics and the actual rectangle2 that is like a "bounding box" rectangle
	// rectangle of 4 points will not suffice for that. Should we remove this union all together and only rely on the transform
	// or fix the rect and use the rect struct in both the renderer and the physics?
	union
	{
		line Line;
		rect2 Rectangle;
		circle Circle;
		triangle Triangle;
	};
};

struct hit_point
{
	u8 Flags;
	u8 Amount;
};

struct entity
{
	entity_id ID;

	entity_type Type;
	u32 Flags;

	vector2 Position;
	vector2 dPosition; //Derivative of Position (Velocity)
	vector2 ddPosition; //Derivative of dPosition (Acceleration)
	entity_movement_blueprint MovementBlueprint;
	
	b32 Collideable;
	entity_physics_blueprint PhysicsBlueprint;
	r32 Width;
	r32 Height;

	u32 HitPointMax;
	hit_point HitPoint;
};

#endif // GAME_ENTITY_H
