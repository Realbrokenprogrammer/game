#ifndef GAME_PHYSICS_H
#define GAME_PHYSICS_H
#pragma once

struct collision_info
{
	b32 IsColliding;

	r32 PenetrationDepth;
	vector2 PenetrationNormal;
};

#endif // GAME_PHYSICS_H
