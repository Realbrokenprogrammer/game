#pragma once

#include "Platform.h"
#include "SDL.h"

Platform *create_platform(int x, int y, int w, int h) {
	Platform *platform = (Platform *)malloc(sizeof(Platform));

	platform->rect = (Rect *)malloc(sizeof(Rect));
	platform->rect->x = x;
	platform->rect->y = y;
	platform->rect->w = w;
	platform->rect->h = h;

	platform->rect_size = 1;

	return platform;
}

void destroy_platform() {

}

int platform_render(Platform *platform, SDL_Renderer *renderer) {

	for (int i = 0; i < platform->rect_size; i++) {
		SDL_Rect rect;
		rect.x = platform->rect->x;
		rect.y = platform->rect->y;
		rect.w = platform->rect->w;
		rect.h = platform->rect->h;

		SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
		SDL_RenderFillRect(renderer, &rect);
	}

	return 0;
}