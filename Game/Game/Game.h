#pragma once
#include <SDL.h>

struct Game 
{
	Game* create_game();
	void destroy_game();
	int game_renderer();
	int game_input();
	int game_update();
	Game_State state;
	SDL_Renderer* renderer;
};

enum Game_State {
	RUNNING,
	PAUSED
};