#ifndef GAME_PHYSICS_H
#define GAME_PHYSICS_H
#pragma once

enum collision_shape
{
	CollisionShape_Line,
	CollisionShape_Circle,
	CollisionShape_Triangle,
	CollisionShape_Rectangle,
	CollisionShape_Polygon
};

struct collision_info
{
	b32 IsColliding;

	r32 PenetrationDepth;
	vector2 PenetrationNormal;
};

#endif // GAME_PHYSICS_H
