#include <SDL.h>
#include <windows.h>
#include <stdio.h>
#include "om_tool.h"
#include "Game.cpp"

struct sdl_offscreen_buffer
{
	SDL_Texture *Texture;
	void *Memory;
	int Width;
	int Height;
	int Pitch;
};

struct sdl_window_dimension
{
	int Width;
	int Height;
};

struct sdl_sound_output
{
	int SamplesPerSecond;
	u32 RunningSampleIndex;
	int BytesPerSample;
	int SecondaryBufferSize;
	r32 tSine;
	int LatencySampleCount;
};

om_global_variable b32 GlobalRunning;
om_global_variable sdl_offscreen_buffer GlobalBackbuffer;

#define MAX_CONTROLLERS 4
#define CONTROLLER_AXIS_LEFT_DEADZONE 7849
om_global_variable SDL_GameController *ControllerHandles[MAX_CONTROLLERS];
om_global_variable SDL_Haptic *RumbleHandles[MAX_CONTROLLERS];

void *
PlatformLoadFile(char *FileName)
{
	return (0);
}

om_internal void
SDLOpenGameControllers()
{
	int MaxJoySticks = SDL_NumJoysticks();
	int ControllerIndex = 0;
	for (int JoyStickIndex = 0; JoyStickIndex < MaxJoySticks; ++JoyStickIndex)
	{
		if (!SDL_IsGameController(JoyStickIndex)) {
			continue;
		}
		if (ControllerIndex >= MAX_CONTROLLERS) {
			break;
		}

		ControllerHandles[ControllerIndex] = SDL_GameControllerOpen(JoyStickIndex);
		SDL_Joystick *JoystickHandle = SDL_GameControllerGetJoystick(ControllerHandles[ControllerIndex]);
		RumbleHandles[ControllerIndex] = SDL_HapticOpenFromJoystick(JoystickHandle);
		if (SDL_HapticRumbleInit(RumbleHandles[ControllerIndex]) != 0) {
			SDL_HapticClose(RumbleHandles[ControllerIndex]);
			RumbleHandles[ControllerIndex] = 0;
		}

		ControllerIndex++;
	}
}

om_internal void
SDLCloseGameControllers()
{
	for (int ControllerIndex = 0; ControllerIndex < MAX_CONTROLLERS; ++ControllerIndex)
	{
		if (ControllerHandles[ControllerIndex])
		{
			if (RumbleHandles[ControllerIndex])
				SDL_HapticClose(RumbleHandles[ControllerIndex]);
			SDL_GameControllerClose(ControllerHandles[ControllerIndex]);
		}
	}
}

om_internal void
SDLInitAudio(i32 SamplesPerSecond, i32 BufferSize)
{
	SDL_AudioSpec AudioSettings = {};

	AudioSettings.freq = SamplesPerSecond;
	AudioSettings.format = AUDIO_S16LSB;
	AudioSettings.channels = 2;
	AudioSettings.samples = 512;

	SDL_OpenAudio(&AudioSettings, 0);

	if (AudioSettings.format != AUDIO_S16LSB)
	{
		SDL_CloseAudio();
	}
}

om_internal void
SDLClearSoundBuffer(sdl_sound_output *SoundOutput)
{
	SDL_ClearQueuedAudio(1);
}

om_internal void
SDLFillSoundBuffer(sdl_sound_output *SoundOutput, int BytesToWrite, game_sound_output_buffer *SoundBuffer)
{
	SDL_QueueAudio(1, SoundBuffer->Samples, BytesToWrite);
}

om_internal void
SDLProcessKeyboardEvent(game_button_state *NewState, b32 IsDown)
{
	if (NewState->EndedDown != IsDown)
	{
		NewState->EndedDown = IsDown;
		++NewState->HalfTransitionCount;
	}
}

om_internal void 
SDLProcessGameControllerButton(game_button_state *OldState, bool Value, game_button_state *NewState)
{
	NewState->EndedDown = Value;
	NewState->HalfTransitionCount = (OldState->EndedDown != NewState->EndedDown) ? 1 : 0;
}

om_internal void 
SDLProcessEvents(game_controller_input *KeyboardController)
{
	SDL_Event Event;
	while (SDL_PollEvent(&Event))
	{
		switch (Event.type)
		{
			case SDL_QUIT:
			{
				GlobalRunning = false;
			} break;
			case SDL_KEYDOWN:
			case SDL_KEYUP:
			{
				SDL_Keycode KeyCode = Event.key.keysym.sym;
				bool IsDown = (Event.key.state == SDL_PRESSED);

				if (Event.key.repeat == 0)
				{
					if (KeyCode == SDLK_w)
					{
						SDLProcessKeyboardEvent(&KeyboardController->Up, IsDown);
					}
					else if (KeyCode == SDLK_a)
					{
						SDLProcessKeyboardEvent(&KeyboardController->Left, IsDown);
					}
					else if (KeyCode == SDLK_s)
					{
						SDLProcessKeyboardEvent(&KeyboardController->Down, IsDown);
					}
					else if (KeyCode == SDLK_d)
					{
						SDLProcessKeyboardEvent(&KeyboardController->Right, IsDown);
					}
					else if (KeyCode == SDLK_q)
					{
						SDLProcessKeyboardEvent(&KeyboardController->LeftShoulder, IsDown);
					}
					else if (KeyCode == SDLK_e)
					{
						SDLProcessKeyboardEvent(&KeyboardController->RightShoulder, IsDown);
					}
					else if (KeyCode == SDLK_UP)
					{
						SDLProcessKeyboardEvent(&KeyboardController->Up, IsDown);
					}
					else if (KeyCode == SDLK_LEFT)
					{
						SDLProcessKeyboardEvent(&KeyboardController->Left, IsDown);
					}
					else if (KeyCode == SDLK_DOWN)
					{
						SDLProcessKeyboardEvent(&KeyboardController->Down, IsDown);
					}
					else if (KeyCode == SDLK_RIGHT)
					{
						SDLProcessKeyboardEvent(&KeyboardController->Right, IsDown);
					}
					else if (KeyCode == SDLK_ESCAPE)
					{

					}
					else if (KeyCode == SDLK_SPACE)
					{

					}

					if (IsDown)
					{
						bool AltKeyWasDown = (Event.key.keysym.mod & KMOD_ALT);
						if (KeyCode == SDLK_F4 && AltKeyWasDown)
						{
							GlobalRunning = false;
						}
						if ((KeyCode == SDLK_RETURN) && AltKeyWasDown)
						{
							SDL_Window *Window = SDL_GetWindowFromID(Event.window.windowID);
							if (Window)
							{
								//SDLToggleFullscreen(Window);
							}
						}
					}
				}
			} break;
			case SDL_WINDOWEVENT:
			{
				switch (Event.window.event)
				{
					case SDL_WINDOWEVENT_SIZE_CHANGED:
					{
					} break;
					case SDL_WINDOWEVENT_FOCUS_GAINED:
					{
					} break;
					case SDL_WINDOWEVENT_EXPOSED:
					{
					} break;
				}
			} break;
		}
	}
}

om_internal sdl_window_dimension
SDLGetWindowDimension(SDL_Window *Window)
{
	sdl_window_dimension Result;

	SDL_GetWindowSize(Window, &Result.Width, &Result.Height);

	return (Result);
}

om_internal void
SDLResizeTexture(sdl_offscreen_buffer *Buffer, SDL_Renderer *Renderer, int Width, int Height)
{
	// TODO(casey): Bulletproof this.
	// Maybe don't free first, free after, then free first if that fails.

		if (Buffer->Memory)
		{
			VirtualFree(Buffer->Memory, 0, MEM_RELEASE);
		}

		Buffer->Width = Width;
		Buffer->Height = Height;

		int BytesPerPixel = 4;
		//Buffer->BytesPerPixel = BytesPerPixel;

		if (Buffer->Texture)
		{
			SDL_DestroyTexture(Buffer->Texture);
		}
		Buffer->Texture = SDL_CreateTexture(Renderer,
			SDL_PIXELFORMAT_ARGB8888,
			SDL_TEXTUREACCESS_STREAMING,
			Buffer->Width,
			Buffer->Height);

		int BitmapMemorySize = (Buffer->Width*Buffer->Height)*BytesPerPixel;
		Buffer->Memory = VirtualAlloc(0, BitmapMemorySize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
		Buffer->Pitch = Width * BytesPerPixel;

		// TODO(casey): Probably clear this to black
}

om_internal void
SDLDisplayBufferInWindow(sdl_offscreen_buffer *Buffer, SDL_Renderer *Renderer, i32 WindowWidth, i32 WindowHeight)
{
	SDL_UpdateTexture(Buffer->Texture, 0, Buffer->Memory, Buffer->Pitch);
	if ((WindowHeight >= Buffer->Height*2) && (WindowWidth >= Buffer->Width*2)) {
		SDL_Rect SrcRect = { 0, 0, Buffer->Width, Buffer->Height };
		SDL_Rect DestRect = { 0, 0, 2 * Buffer->Width, 2 * Buffer->Height };
		SDL_RenderCopy(Renderer,
					   Buffer->Texture,
					   &SrcRect,
					   &DestRect);
	}
	else
	{
		int OffsetX = 10;
		int OffsetY = 10;

		SDL_SetRenderDrawColor(Renderer, 0, 0, 0, 255);
		SDL_Rect BlackRects[4] = {
			{0, 0, WindowWidth, OffsetY},
			{0, OffsetY + Buffer->Height, WindowWidth, WindowHeight},
			{0, 0, OffsetX, WindowHeight},
			{OffsetX + Buffer->Width, 0, WindowWidth, WindowHeight}
		};
		SDL_RenderFillRects(Renderer, BlackRects, OM_ARRAYCOUNT(BlackRects));

		// NOTE(casey): For prototyping purposes, we're going to always blit
		// 1-to-1 pixels to make sure we don't introduce artifacts with
		// stretching while we are learning to code the renderer!
		SDL_Rect SrcRect = { 0, 0, Buffer->Width, Buffer->Height };
		SDL_Rect DestRect = { OffsetX, OffsetY, Buffer->Width, Buffer->Height };
		SDL_RenderCopy(Renderer,
					   Buffer->Texture,
					   &SrcRect,
					   &DestRect);
	}

	SDL_RenderPresent(Renderer);
}

inline u64
SDLGetWallClock(void)
{
	u64 Result;

	Result = SDL_GetPerformanceCounter();

	return (Result);
}

om_internal r32
SDLGetSecondsElapsed(u64 Start, u64 End, r32 GlobalPerfCountFrequency)
{
	r32 Result;

	Result = ((r32)(End - Start) / GlobalPerfCountFrequency);

	return (Result);
}

int main(int argc, char *argv[]) {

	u64 PerfCountFrequencyResult = SDL_GetPerformanceFrequency();

	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER | SDL_INIT_HAPTIC | SDL_INIT_AUDIO);

	SDLOpenGameControllers();

	// Create SDL Window.
	SDL_Window *Window = SDL_CreateWindow("Game",
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		1920,
		1080,
		SDL_WINDOW_RESIZABLE);

	if (Window) {
		// TODO: Show cursor if debug global

		SDL_Renderer *Renderer = SDL_CreateRenderer(Window, -1, SDL_RENDERER_PRESENTVSYNC);

		if (Renderer) {
			SDLResizeTexture(&GlobalBackbuffer, Renderer, 1920, 1080);

			sdl_sound_output SoundOutput = {};
			SoundOutput.SamplesPerSecond = 48000;
			SoundOutput.RunningSampleIndex = 0;
			SoundOutput.BytesPerSample = sizeof(i16) * 2;
			SoundOutput.SecondaryBufferSize = SoundOutput.SamplesPerSecond * SoundOutput.BytesPerSample;
			SoundOutput.LatencySampleCount = SoundOutput.SamplesPerSecond / 15;
			int SquareWavePeriod = SoundOutput.SamplesPerSecond / 256;
			int HalfSquareWavePeriod = SquareWavePeriod / 2;

			SDLInitAudio(SoundOutput.SamplesPerSecond, SoundOutput.SecondaryBufferSize);
			SDLClearSoundBuffer(&SoundOutput);
			SDL_PauseAudio(0);

			GlobalRunning = true;

			//TODO: Pool with bitmap VirtualAlloc.
			i16 *Samples = (i16 *)VirtualAlloc(0, SoundOutput.SecondaryBufferSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

#ifdef OM_DEBUG
			void *BaseAddress = (void *)om_terabytes(2);
#else
			void *BaseAddress = 0;
#endif
			game_memory GameMemory = {};
			GameMemory.PermanentStorageSize = om_megabytes(64);
			GameMemory.TransientStorageSize = om_gigabytes((u64)4);

			u64 TotalSize = GameMemory.PermanentStorageSize + GameMemory.TransientStorageSize;
			GameMemory.PermanentStorage = VirtualAlloc(BaseAddress, TotalSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE); //TODO: Option for VirtualAlloc?

			GameMemory.TransientStorage = ((u8 *)GameMemory.PermanentStorage + GameMemory.PermanentStorageSize);

			if (Samples && GameMemory.PermanentStorage && GameMemory.TransientStorage)
			{
				game_input Input[2] = {};
				game_input *NewInput = &Input[0];
				game_input *OldInput = &Input[1];

				//TODO: ??
				u64 LastCounter = SDLGetWallClock();
				u64 FlipWallClock = SDLGetWallClock();

				i64 LastCycleCount = __rdtsc();
				while (GlobalRunning) {

					// TODO(casey): Zeroing macro
					// TODO(casey): We can't zero everything because the up/down state will
					// be wrong!!!
					/*game_controller_input *OldKeyboardController = GetController(OldInput, 0);
					game_controller_input *NewKeyboardController = GetController(NewInput, 0);
					*NewKeyboardController = {};
					NewKeyboardController->IsConnected = true;
					for (int ButtonIndex = 0;
						ButtonIndex < OM_ARRAYCOUNT(NewKeyboardController->Buttons);
						++ButtonIndex)
					{
						NewKeyboardController->Buttons[ButtonIndex].EndedDown =
							OldKeyboardController->Buttons[ButtonIndex].EndedDown;
					}*/

					//SDLProcessEvents(NewKeyboardController);

					int MaxControllerCount = MAX_CONTROLLERS;
					if (MaxControllerCount > OM_ARRAYCOUNT(NewInput->Controllers))
					{
						MaxControllerCount = OM_ARRAYCOUNT(NewInput->Controllers);
					}

					for (DWORD ControllerIndex = 0; ControllerIndex < MaxControllerCount; ControllerIndex++)
					{
						i32 OurControllerIndex = ControllerIndex + 1;
						game_controller_input *OldController = &OldInput->Controllers[OurControllerIndex];
						game_controller_input *NewController = &NewInput->Controllers[OurControllerIndex];

						SDL_GameController *Controller = ControllerHandles[ControllerIndex];
						if (Controller && SDL_GameControllerGetAttached(Controller))
						{
							NewController->IsConnected = true;
							NewController->IsAnalog = OldController->IsAnalog;

							// NOTE(casey): This controller is plugged in
							i16 AxisLX = SDL_GameControllerGetAxis(Controller, SDL_CONTROLLER_AXIS_LEFTX);
							i16 AxisLY = SDL_GameControllerGetAxis(Controller, SDL_CONTROLLER_AXIS_LEFTY);

							// TODO(casey): This is a square deadzone, check XInput to
								// verify that the deadzone is "round" and show how to do
								// round deadzone processing.
							/*NewController->StickAverageX = SDLProcessGameControllerAxisValue(
								AxisLX, CONTROLLER_AXIS_LEFT_DEADZONE);
							NewController->StickAverageY = SDLProcessGameControllerAxisValue(
								AxisLY, CONTROLLER_AXIS_LEFT_DEADZONE);
							if ((NewController->StickAverageX != 0.0f) ||
								(NewController->StickAverageY != 0.0f))
							{
								NewController->IsAnalog = true;
							}

							if (SDL_GameControllerGetButton(Controller, SDL_CONTROLLER_BUTTON_DPAD_UP))
							{
								NewController->StickAverageY = 1.0f;
								NewController->IsAnalog = false;
							}

							if (SDL_GameControllerGetButton(Controller, SDL_CONTROLLER_BUTTON_DPAD_DOWN))
							{
								NewController->StickAverageY = -1.0f;
								NewController->IsAnalog = false;
							}

							if (SDL_GameControllerGetButton(Controller, SDL_CONTROLLER_BUTTON_DPAD_LEFT))
							{
								NewController->StickAverageX = -1.0f;
								NewController->IsAnalog = false;
							}

							if (SDL_GameControllerGetButton(Controller, SDL_CONTROLLER_BUTTON_DPAD_RIGHT))
							{
								NewController->StickAverageX = 1.0f;
								NewController->IsAnalog = false;
							}*/

							r32 Threshold = 0.5f;
							/*SDLProcessGameControllerButton(
								&OldController->MoveLeft,
								NewController->StickAverageX < -Threshold,
								&NewController->MoveLeft);
							SDLProcessGameControllerButton(
								&OldController->MoveRight,
								NewController->StickAverageX > Threshold,
								&NewController->MoveRight);
							SDLProcessGameControllerButton(
								&OldController->MoveDown,
								NewController->StickAverageY < -Threshold,
								&NewController->MoveDown);
							SDLProcessGameControllerButton(
								&OldController->MoveUp,
								NewController->StickAverageY > Threshold,
								&NewController->MoveUp);*/

							SDLProcessGameControllerButton(&OldController->Down,
								SDL_GameControllerGetButton(Controller, SDL_CONTROLLER_BUTTON_A),
								&NewController->Down);
							SDLProcessGameControllerButton(&OldController->Right,
								SDL_GameControllerGetButton(Controller, SDL_CONTROLLER_BUTTON_B),
								&NewController->Right);
							SDLProcessGameControllerButton(&OldController->Left,
								SDL_GameControllerGetButton(Controller, SDL_CONTROLLER_BUTTON_X),
								&NewController->Left);
							SDLProcessGameControllerButton(&OldController->Up,
								SDL_GameControllerGetButton(Controller, SDL_CONTROLLER_BUTTON_Y),
								&NewController->Up);
							SDLProcessGameControllerButton(&OldController->LeftShoulder,
								SDL_GameControllerGetButton(Controller, SDL_CONTROLLER_BUTTON_LEFTSHOULDER),
								&NewController->LeftShoulder);
							SDLProcessGameControllerButton(&OldController->RightShoulder,
								SDL_GameControllerGetButton(Controller, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER),
								&NewController->RightShoulder);

							/*SDLProcessGameControllerButton(&OldController->Start,
								SDL_GameControllerGetButton(Controller, SDL_CONTROLLER_BUTTON_START),
								&NewController->Start);
							SDLProcessGameControllerButton(&OldController->Back,
								SDL_GameControllerGetButton(Controller, SDL_CONTROLLER_BUTTON_BACK),
								&NewController->Back);*/
						}
						else
						{
							// NOTE(casey): The controller is not available
							NewController->IsConnected = false;
						}
					}

					b32 SoundIsValid = true;
					
					//TODO: More robust BytesToWrite..
					int BytesToWrite = 800 * SoundOutput.BytesPerSample;

					game_sound_output_buffer SoundBuffer = {};
					SoundBuffer.SamplesPerSecond = SoundOutput.SamplesPerSecond;
					SoundBuffer.SampleCount = BytesToWrite / SoundOutput.BytesPerSample;
					SoundBuffer.Samples = Samples;

					game_offscreen_buffer Buffer = {};
					Buffer.Memory = GlobalBackbuffer.Memory;
					Buffer.Width = GlobalBackbuffer.Width;
					Buffer.Height = GlobalBackbuffer.Height;
					Buffer.Pitch = GlobalBackbuffer.Pitch;

					GameUpdateAndRender(&GameMemory, NewInput, &Buffer, &SoundBuffer);

					//NOTE: Sound output test.
					if (SoundIsValid)
					{
						SDLFillSoundBuffer(&SoundOutput, BytesToWrite, &SoundBuffer);
					}

					sdl_window_dimension Dimension = SDLGetWindowDimension(Window);
					SDLDisplayBufferInWindow(&GlobalBackbuffer, Renderer,
						Dimension.Width, Dimension.Height);

					i64 EndCycleCount = __rdtsc();

					u64 EndCounter = SDLGetWallClock();
					r32 MSPerFrame = 1000.0f*SDLGetSecondsElapsed(LastCounter, EndCounter, PerfCountFrequencyResult);
					LastCounter = EndCounter;

					FlipWallClock = SDLGetWallClock();

					game_input *Temp = NewInput;
					NewInput = OldInput;
					OldInput = Temp;

					LastCounter = EndCounter;
					LastCycleCount = EndCycleCount;
				}
			}
			else
			{

			}
		}
		else
		{

		}
	}
	else
	{

	}
	return (0);
}