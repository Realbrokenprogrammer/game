#include <SDL.h>
#include <stdio.h>

//Screen dimension constants
const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;

bool isRunning = true;

int main(int argc, char* args[])
{
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

	//TODO: Support for SDL joysticks

	//TODO: Initialize Sound
	
	SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
	SDL_Rect rect;
	rect.h = SCREEN_HEIGHT;
	rect.w = SCREEN_WIDTH;
	rect.x = 0;
	rect.y = 0;

	const Uint8 *const keyboardState = SDL_GetKeyboardState(NULL);

	SDL_Event e;
	while (isRunning) {
		SDL_RenderFillRect(renderer, &rect);
		SDL_RenderPresent(renderer);

		if (keyboardState[SDL_SCANCODE_A]) {
			SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
		}

		if (SDL_PollEvent(&e)) {
			switch (e.type) {
			case SDL_QUIT:
				isRunning = false;
				break;
			case SDL_KEYDOWN:
				switch (e.key.keysym.sym) {
				case SDLK_k:
					SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
					break;
				}
				break;
			}
		}
		
	}

	//Destroy Renderer
	SDL_DestroyRenderer(renderer);

	//Destroy window
	SDL_DestroyWindow(window);

	//Quit SDL subsystems
	SDL_Quit();

	return 0;
}