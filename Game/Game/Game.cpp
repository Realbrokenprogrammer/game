#include "Game.h"

Game *create_game(SDL_Renderer *renderer)
{
	Game *game = (Game *) malloc(sizeof(Game));
	if (game == NULL) {
		//TODO: Throw some kind of error?
		return NULL;
	}

	game->renderer = renderer;
	game->state = GAME_STATE_RUNNING;

	return game;
}

void destroy_game(Game *game)
{
	//TODO: Check that game isn't null
	free(game);
}

int game_render(Game *game)
{
	if (game->state == GAME_STATE_QUIT) {
		return 0;
	}

	// Do render stuff here.
	SDL_Rect rect;
	rect.h = 100;
	rect.w = 100;
	rect.x = 0;
	rect.y = 0;


	SDL_SetRenderDrawColor(game->renderer, 255, 0, 0, 255);
	SDL_RenderDrawRect(game->renderer, &rect);


	return 0;
}

int game_update(Game* game, float delta_time)
{
	if (game->state == GAME_STATE_QUIT) {
		return 0;
	}

	if (game->state == GAME_STATE_RUNNING) {
		// Do game update here
	}

	return 0;
}

int game_handle_running_event(Game *game, const SDL_Event *e) {
	switch (e->type) {
		case SDL_QUIT:
		{
			game->state = GAME_STATE_QUIT;
		} break;
	}

	return 0;
}

int game_handle_event(Game *game, const SDL_Event *e) {

	switch (game->state) {
		case GAME_STATE_RUNNING: 
		{
			return game_handle_running_event(game, e);
		} break;
		case GAME_STATE_PAUSED:
		{

		} break;
		default: break;
	}

	return 0;
}

int game_input(Game *game, const Uint8 *const keyboard_state, SDL_Joystick *joystick)
{
	if (game->state == GAME_STATE_QUIT || game->state == GAME_STATE_PAUSED) {
		return 0;
	}

	return 0;
}

bool game_is_running(const Game *game) {
	return game->state == GAME_STATE_QUIT;
}