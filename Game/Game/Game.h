#pragma once
#include <SDL.h>

enum Game_State {
	RUNNING,
	PAUSED
};

struct Game 
{
	Game_State state;
	SDL_Renderer* renderer;
};

Game* create_game();
void destroy_game();
int game_renderer();
int game_input();
int game_update();