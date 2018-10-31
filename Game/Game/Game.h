#pragma once
#include <SDL.h>
#include "Level.h"

enum Game_State {
	GAME_STATE_RUNNING,
	GAME_STATE_PAUSED,
	GAME_STATE_QUIT
};

struct Game 
{
	Game_State state;
	SDL_Renderer* renderer;
	Level* level;
};

Game* create_game(SDL_Renderer *renderer);
void destroy_game(Game *game);
int game_render(Game *game);
int game_update(Game* game, float delta_time);
int game_handle_event(Game *game, const SDL_Event *e);
int game_input(Game *game, const Uint8 *const keyboard_state, SDL_Joystick *joystick);

bool game_is_running(const Game *game);