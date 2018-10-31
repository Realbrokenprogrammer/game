#pragma once

#include <vector>
#include "Entity.h"

struct Level {
	std::vector<Entity *> m_entities;
};

Level *create_level();
void destroy_level(Level *level);

int level_render();
int level_update();
int level_handle_event();
int level_input();