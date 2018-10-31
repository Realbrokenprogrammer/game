#pragma once

#include <vector>
#include "Entity.h"

struct Level {
	//TODO: Use some sort of array instead so we don't have to use 'new' to alloc this class.
	std::vector<Platform *> m_entities;
};

Level *create_level();
void destroy_level(Level *level);

int level_render(Level *level, SDL_Renderer *renderer);
int level_update();
int level_handle_event();
int level_input();