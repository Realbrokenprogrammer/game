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
	EntityType_WaterTile
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

enum collision_shape
{
	CollisionShape_Line,
	CollisionShape_Rectangle,
	CollisionShape_Circle,
	CollisionShape_Triangle,
	CollisionShape_Polygon
};

struct entity_physics_blueprint
{
	b32 Solid;

	collision_shape CollisionShape;
	union
	{
		line Line;
		rect2 Rectangle;
		circle Circle;
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

	//TODO: Camera positions

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
