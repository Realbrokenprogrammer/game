#include <SDL.h>
#include <stdio.h>
#include "om_tool.h"
#include "Game.h"

//Screen dimension constants
const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;

bool isRunning = true;



int main(int argc, char* args[])
{
	int fps = 60;

	//The window we'll be rendering to
	SDL_Window* window = NULL;

	//The surface contained by the window
	SDL_Surface* screenSurface = NULL;


	//Initialize SDL
	if (SDL_Init(SDL_INIT_EVERYTHING) < 0)
	{
		printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
		return 1;
	}

	SDL_ShowCursor(SDL_DISABLE);

	window = SDL_CreateWindow("Game", 100, 100, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
	if (window == NULL) {
		printf("SDL Couldn't create window! SDL_ERROR: %s\n", SDL_GetError());
		return 1;
	}

	SDL_Renderer* const renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	if (renderer == NULL) {
		printf("SDL Couldn't create renderer! SDL_ERROR: %s\n", SDL_GetError());
		return 1;
	}

	if (SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND) < 0) {
		printf("SDL Couldn't set blend mode! SDL_ERROR: %s\n", SDL_GetError());
		return 1;
	}

	//TODO(#1): Support for SDL joysticks
	SDL_Joystick *joyStick = NULL;

	//TODO(#2): Initialize Sound

	Game *const game = create_game(renderer);

	const Uint8 *const keyboardState = SDL_GetKeyboardState(NULL);

	SDL_Event e;
	const int64_t delta_time = (int64_t)roundf(1000.0f / 60.0f);
	int64_t render_time = (int64_t)roundf(1000.0f / (float)fps);
	while (!game_is_running(game)) {
		const int64_t begin_frame_time = (int64_t)SDL_GetTicks();

		while (!game_is_running(game) && SDL_PollEvent(&e)) {
			if (game_handle_event(game, &e) < 0) {
				//TODO: Error handling
			}
		}

		if (game_input(game, keyboardState, joyStick) < 0) {
			//TODO: Error handling
		}

		if (game_update(game, (float) delta_time * 0.001f) < 0) {
			//TODO: Error handling
		}

		//TODO: Game sound

		render_time -= delta_time;
		if (render_time <= 0) {

			if (game_render(game) < 0) {
				//TODO: Error handling
			}

			SDL_RenderPresent(renderer);
			render_time = (int64_t)roundf(1000.0f / (float)fps);
		}

		const int64_t end_frame_time = (int64_t)SDL_GetTicks();
		SDL_Delay((unsigned int)OM_MAX(10, delta_time - (end_frame_time - begin_frame_time)));
	}

	destroy_game(game);

	//Destroy Renderer
	SDL_DestroyRenderer(renderer);

	//Destroy window
	SDL_DestroyWindow(window);

	//Quit SDL subsystems
	SDL_Quit();

	return 0;
}