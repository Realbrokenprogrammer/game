#pragma once

#include "SDL.h"
#include "Rectangle.h"

//TODO: Rewrite this to be an Entity, Perhaps use an Entity Manager to handle entities.
struct Platform {
	Rect* rect;
	//Color *colors;
	//SDL_Texture *texture;
	size_t rect_size;
};

Platform *create_platform(int x, int y, int w, int h);
void destroy_platform();

int platform_render(Platform *platform, SDL_Renderer *renderer);
