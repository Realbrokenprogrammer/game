#pragma once

#include "Level.h"
#include "Platform.h"

Level *create_level() {

	Level *level = new Level;
	if (level == NULL) {
		//TODO: Error handling
		return NULL;
	}

	//TODO: This is temporary testing code.
	Platform *p1 = create_platform(0, 0, 10, 10);
	Platform *p2 = create_platform(20, 0, 10, 10);
	Platform *p3 = create_platform(0, 100, 10, 10);

	//level->m_entities = std::vector<Platform *>();
	level->m_entities.push_back(p1);
	level->m_entities.push_back(p2);
	level->m_entities.push_back(p3);

	return level;
}

void destroy_level(Level *level) {
	//TODO: Clean up level and its entities.
}

int level_render(Level *level, SDL_Renderer *renderer) {

	for (auto &p : level->m_entities) {
		platform_render(p, renderer);
	}

	return 0;
}

int level_update() {
	return 0;
}

int level_handle_event() {
	return 0;
}

int level_input() {
	return 0;
}