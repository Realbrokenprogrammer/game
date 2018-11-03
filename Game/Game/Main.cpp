// Define to enable debugging functionality
#define OM_DEBUG 1

#include "Platform.h"
#include "Game_Intristics.h"
#include "Game_Math.h"

#include <SDL.h>

#include <windows.h>
#include <stdio.h>

#include "Game.h"
#include "Game.cpp"

#include "Main.h" //TODO: Main.h should be renamed, same with Main.cpp.

/*
	TODO:
		* Saved game locations
		* Get handle to our own executable file
		* Asset loading path
		* Threading (Launch a thread)
		* Raw Input (support for multiple keyboards
		* Sleep/timeBeginPeriod
		* ClipCursor() (Multimonitor support)
		* Fullscreen support
		* WM_SETCURSOR (Control cursor visibility)
		* QueryCancelAutoplay
		* WM_ACTIVEAPP (When this app is not the active one)
		* Blit speed improvements (BitBlt)
		* Hadware acceleration (OpenGL)
		* GetKeyboardLayout (International WASD support)
*/

om_global_variable b32 GlobalRunning;
om_global_variable sdl_offscreen_buffer GlobalBackbuffer;
om_global_variable sdl_audio_ring_buffer GlobalSecondaryBuffer;
om_global_variable u64 GlobalPerfCountFrequency;

#define MAX_CONTROLLERS 4
#define CONTROLLER_AXIS_LEFT_DEADZONE 7849
om_global_variable SDL_GameController *ControllerHandles[MAX_CONTROLLERS];
om_global_variable SDL_Haptic *RumbleHandles[MAX_CONTROLLERS];

om_internal debug_read_file_result
DEBUGPlatformReadEntireFile(char *FileName)
{
	debug_read_file_result Result = {};

	HANDLE FileHandle = CreateFile(FileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL);
	if (FileHandle != INVALID_HANDLE_VALUE) 
	{
		LARGE_INTEGER FileSize;
		if (GetFileSizeEx(FileHandle, &FileSize))
		{
			//TODO: Defines for maximum values.
			u32 FileSize32 = SafeTruncateUInt64(FileSize.QuadPart);
			Result.Contents = VirtualAlloc(0, FileSize32, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
			if (Result.Contents)
			{ 
				DWORD BytesRead;
				if (ReadFile(FileHandle, Result.Contents, FileSize32, &BytesRead, NULL) && 
					(FileSize32 == BytesRead))
				{
					// File read successfully.
					Result.ContentsSize = FileSize32;
				}
				else
				{
					//TODO: Logging
					DEBUGPlatformFreeFileMemory(Result.Contents);
					Result.Contents = 0;
				}
			}
			else
			{
				//TODO: Logging
			}
		}
		else
		{
			//TODO: Logging
		}

		CloseHandle(FileHandle);
	}

	return (Result);
}

om_internal void 
DEBUGPlatformFreeFileMemory(void *Memory)
{
	if (Memory)
	{
		VirtualFree(Memory, NULL, MEM_RELEASE);
	}
}

om_internal b32 
DEBUGPlatformWriteEntireFile(char *FileName, u32 MemorySize, void *Memory)
{
	b32 Result = false;

	HANDLE FileHandle = CreateFile(FileName, GENERIC_WRITE, NULL, NULL, CREATE_ALWAYS, NULL, NULL);
	if (FileHandle != INVALID_HANDLE_VALUE) 
	{
		DWORD BytesWritten;
		if (WriteFile(FileHandle, Memory, MemorySize, &BytesWritten, NULL))
		{
			// File read successfully.
			Result = (BytesWritten == MemorySize);
		}
		else
		{
			//TODO: Logging
		}

		CloseHandle(FileHandle);
	}
	else
	{
		//TODO: Logging
	}


	return (Result);
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
SDLAudioCallback(void *UserData, u8 *AudioData, int Length)
{
	sdl_audio_ring_buffer *RingBuffer = (sdl_audio_ring_buffer *)UserData;

	int Region1Size = Length;
	int Region2Size = 0;
	if ((RingBuffer->PlayCursor + Length) > RingBuffer->Size) 
	{
		Region1Size = RingBuffer->Size - RingBuffer->PlayCursor;
		Region2Size = Length - Region1Size;
	}

	memcpy(AudioData, (u8*)RingBuffer->Data + RingBuffer->PlayCursor, Region1Size);
	memcpy(AudioData + Region1Size, RingBuffer->Data, Region2Size);
	RingBuffer->PlayCursor = (RingBuffer->PlayCursor + Length) % RingBuffer->Size;
	RingBuffer->WriteCursor = (RingBuffer->PlayCursor + Length) % RingBuffer->Size;
}

om_internal void
SDLInitAudio(i32 SamplesPerSecond, i32 BufferSize)
{
	SDL_AudioSpec AudioSettings = {};

	AudioSettings.freq = SamplesPerSecond;
	AudioSettings.format = AUDIO_S16LSB;
	AudioSettings.channels = 2;
	AudioSettings.samples = 512;
	AudioSettings.callback = &SDLAudioCallback;
	AudioSettings.userdata = &GlobalSecondaryBuffer;

	GlobalSecondaryBuffer.Size = BufferSize;
	GlobalSecondaryBuffer.Data = calloc(BufferSize, 1);
	GlobalSecondaryBuffer.PlayCursor = GlobalSecondaryBuffer.WriteCursor = 0;

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
SDLFillSoundBuffer(sdl_sound_output *SoundOutput, int ByteToLock, int BytesToWrite, 
	game_sound_output_buffer *SourceBuffer)
{
	void *Region1 = (u8 *)GlobalSecondaryBuffer.Data + ByteToLock;
	int Region1Size = BytesToWrite;
	if (Region1Size + ByteToLock > SoundOutput->SecondaryBufferSize)
	{
		Region1Size = SoundOutput->SecondaryBufferSize - ByteToLock;
	}
	void *Region2 = GlobalSecondaryBuffer.Data;
	int Region2Size = BytesToWrite - Region1Size;

	// TODO(casey): Collapse these two loops
	int Region1SampleCount = Region1Size / SoundOutput->BytesPerSample;
	i16 *DestSample = (i16 *)Region1;
	i16 *SourceSample = SourceBuffer->Samples;
	for (int SampleIndex = 0;
		SampleIndex < Region1SampleCount;
		++SampleIndex)
	{
		*DestSample++ = *SourceSample++;
		*DestSample++ = *SourceSample++;
		++SoundOutput->RunningSampleIndex;
	}

	int Region2SampleCount = Region2Size / SoundOutput->BytesPerSample;
	DestSample = (i16 *)Region2;
	for (int SampleIndex = 0;
		SampleIndex < Region2SampleCount;
		++SampleIndex)
	{
		*DestSample++ = *SourceSample++;
		*DestSample++ = *SourceSample++;
		++SoundOutput->RunningSampleIndex;
	}
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
	NewState->HalfTransitionCount = (OldState->EndedDown == NewState->EndedDown) ? 1 : 0;
}

om_internal r32
SDLProcessGameControllerAxisValue(i16 Value, i16 DeadZoneThreshold)
{
	r32 Result = 0;

	if (Value < -DeadZoneThreshold)
	{
		Result = (r32)((Value + DeadZoneThreshold) / (32768.0f - DeadZoneThreshold));
	}
	else if (Value > DeadZoneThreshold)
	{
		Result = (r32)((Value - DeadZoneThreshold) / (32767.0f - DeadZoneThreshold));
	}

	return (Result);
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
						OutputDebugStringA("IsDown W");
						SDLProcessKeyboardEvent(&KeyboardController->MoveUp, IsDown);
					}
					else if (KeyCode == SDLK_a)
					{
						SDLProcessKeyboardEvent(&KeyboardController->MoveLeft, IsDown);
					}
					else if (KeyCode == SDLK_s)
					{
						SDLProcessKeyboardEvent(&KeyboardController->MoveDown, IsDown);
					}
					else if (KeyCode == SDLK_d)
					{
						SDLProcessKeyboardEvent(&KeyboardController->MoveRight, IsDown);
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
						SDLProcessKeyboardEvent(&KeyboardController->ActionUp, IsDown);
					}
					else if (KeyCode == SDLK_LEFT)
					{
						SDLProcessKeyboardEvent(&KeyboardController->ActionLeft, IsDown);
					}
					else if (KeyCode == SDLK_DOWN)
					{
						SDLProcessKeyboardEvent(&KeyboardController->ActionDown, IsDown);
					}
					else if (KeyCode == SDLK_RIGHT)
					{
						SDLProcessKeyboardEvent(&KeyboardController->ActionRight, IsDown);
					}
					else if (KeyCode == SDLK_ESCAPE)
					{
						GlobalRunning = false;
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
		Buffer->BytesPerPixel = BytesPerPixel;

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
SDLGetSecondsElapsed(u64 Start, u64 End)
{
	r32 Result;

	Result = ((r32)(End - Start) / (r32)GlobalPerfCountFrequency);

	return (Result);
}

int main(int argc, char *argv[]) {

	GlobalPerfCountFrequency = SDL_GetPerformanceFrequency();
	
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER | SDL_INIT_HAPTIC | SDL_INIT_AUDIO);

	SDLOpenGameControllers();

	//SDLRezieBuffer(&GlobalBackBuffer, 192, 108);
	//SDLRezieBuffer(&GlobalBackBuffer, 480, 270);
	//SDLRezieBuffer(&GlobalBackBuffer, 960, 540);
	//SDLResizeBuffer(&GlobalBackBuffer, 1920, 1080);

	// Create SDL Window.
	SDL_Window *Window = SDL_CreateWindow("Game",
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		1920,
		1080,
		SDL_WINDOW_RESIZABLE);

	if (Window) {
		// TODO: Show cursor if debug global

		u32 MaxQuadCountPerFrame = (1 << 18);

		SDL_Renderer *Renderer = SDL_CreateRenderer(Window, -1, SDL_RENDERER_PRESENTVSYNC);

		if (Renderer) {
			SDLResizeTexture(&GlobalBackbuffer, Renderer, 1920, 1080);
			
			int MonitorRefreshHz = 60;
			int DisplayIndex = SDL_GetWindowDisplayIndex(Window);
			SDL_DisplayMode Mode = {};
			int DisplayModeResult = SDL_GetDesktopDisplayMode(DisplayIndex, &Mode);
			if (DisplayModeResult == 0 && Mode.refresh_rate > 1) 
			{
				MonitorRefreshHz = Mode.refresh_rate;
			}
			r32 GameUpdateHz = (r32)(MonitorRefreshHz / 2);

			sdl_sound_output SoundOutput = {};
			SoundOutput.SamplesPerSecond = 48000;
			SoundOutput.RunningSampleIndex = 0;
			SoundOutput.BytesPerSample = sizeof(i16) * 2;
			SoundOutput.SecondaryBufferSize = SoundOutput.SamplesPerSecond * SoundOutput.BytesPerSample;
			SoundOutput.SafetyBytes = (int)(((r32)SoundOutput.SamplesPerSecond*(r32)SoundOutput.BytesPerSample / GameUpdateHz)); // / 2.0f);
			
			SDLInitAudio(SoundOutput.SamplesPerSecond, SoundOutput.SecondaryBufferSize);
			SDLClearSoundBuffer(&SoundOutput); //TODO: Redundant?
			SDL_PauseAudio(0);

			GlobalRunning = true;

			//TODO: Pool with bitmap VirtualAlloc.
			u32 MaxPossibleOverrun = 2 * 8 * sizeof(u16);
			i16 *Samples = (i16 *)VirtualAlloc(0, SoundOutput.SecondaryBufferSize + MaxPossibleOverrun, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

#ifdef OM_DEBUG
			void *BaseAddress = (void *)om_terabytes(2);
#else
			void *BaseAddress = 0;
#endif
			game_memory GameMemory = {};
			GameMemory.PermanentStorageSize = om_megabytes(64);
			GameMemory.TransientStorageSize = om_gigabytes(1);

			u64 TotalSize = GameMemory.PermanentStorageSize + GameMemory.TransientStorageSize;
			GameMemory.PermanentStorage = VirtualAlloc(BaseAddress, (size_t)TotalSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE); //TODO: Option for VirtualAlloc?

			GameMemory.TransientStorage = ((u8 *)GameMemory.PermanentStorage + GameMemory.PermanentStorageSize);

			if (Samples && GameMemory.PermanentStorage && GameMemory.TransientStorage)
			{
				game_input Input[2] = {};
				game_input *NewInput = &Input[0];
				game_input *OldInput = &Input[1];
				
				u64 LastCounter = SDLGetWallClock();
				u64 FlipWallClock = SDLGetWallClock();

				int DebugTimeMarkerIndex = 0;
				sdl_debug_time_marker DebugTimeMarkers[30] = { 0 };

				b32 SoundIsValid = false;

				SDL_ShowWindow(Window);
				u32 ExpectedFramesPerUpdate = 1;
				r32 TargetSecondsPerFrame = (r32)ExpectedFramesPerUpdate / (r32)GameUpdateHz;
				while (GlobalRunning) {

					// TODO(casey): We can't zero everything because the up/down state will
					// be wrong!!!
					// TODO(casey): Zeroing macro
					game_controller_input *OldKeyboardController = GetController(OldInput, 0);
					game_controller_input *NewKeyboardController = GetController(NewInput, 0); 
					*NewKeyboardController = {};

					NewKeyboardController->IsConnected = true;
					for (int ButtonIndex = 0;
						ButtonIndex < OM_ARRAYCOUNT(NewKeyboardController->Buttons);
						++ButtonIndex)
					{
						NewKeyboardController->Buttons[ButtonIndex].EndedDown =
							OldKeyboardController->Buttons[ButtonIndex].EndedDown;
					}

					SDLProcessEvents(NewKeyboardController);

					DWORD MaxControllerCount = MAX_CONTROLLERS;
					if (MaxControllerCount > (OM_ARRAYCOUNT(NewInput->Controllers) - 1))
					{
						MaxControllerCount = (OM_ARRAYCOUNT(NewInput->Controllers) - 1);
					}

					//TODO: Don't poll disconnected controllers.
					for (DWORD ControllerIndex = 0; ControllerIndex < MaxControllerCount; ++ControllerIndex)
					{
						i32 OurControllerIndex = ControllerIndex + 1;
						game_controller_input *OldController = GetController(OldInput, OurControllerIndex);
						game_controller_input *NewController = GetController(NewInput, OurControllerIndex);

						SDL_GameController *Controller = ControllerHandles[ControllerIndex];
						if (Controller && SDL_GameControllerGetAttached(Controller))
						{
							NewController->IsConnected = true;
							NewController->IsAnalog = OldController->IsAnalog;

							// NOTE(casey): This controller is plugged in
							i16 AxisTL = SDL_GameControllerGetAxis(Controller, SDL_CONTROLLER_AXIS_TRIGGERLEFT);
							i16 AxisTR = SDL_GameControllerGetAxis(Controller, SDL_CONTROLLER_AXIS_TRIGGERRIGHT);

							i16 TriggerMax = AxisTL;
							if (TriggerMax < AxisTR)
							{
								TriggerMax = AxisTR;
							}

							//TODO: ClutchMax here

							i16 AxisLX = SDL_GameControllerGetAxis(Controller, SDL_CONTROLLER_AXIS_LEFTX);
							i16 AxisLY = SDL_GameControllerGetAxis(Controller, SDL_CONTROLLER_AXIS_LEFTY);

							// TODO(casey): This is a square deadzone, check XInput to
							// verify that the deadzone is "round" and show how to do
							// round deadzone processing.
							NewController->StickAverageX = SDLProcessGameControllerAxisValue(AxisLX, CONTROLLER_AXIS_LEFT_DEADZONE);
							NewController->StickAverageY = -SDLProcessGameControllerAxisValue(AxisLY, CONTROLLER_AXIS_LEFT_DEADZONE);
							if ((NewController->StickAverageX != 0.0f) || (NewController->StickAverageY != 0.0f))
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
							}


							r32 Threshold = 0.5f;
							SDLProcessGameControllerButton(
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
								&NewController->MoveUp);
							
							SDLProcessGameControllerButton(&OldController->ActionDown,
								SDL_GameControllerGetButton(Controller, SDL_CONTROLLER_BUTTON_A),
								&NewController->ActionDown);
							SDLProcessGameControllerButton(&OldController->ActionRight,
								SDL_GameControllerGetButton(Controller, SDL_CONTROLLER_BUTTON_B),
								&NewController->ActionRight);
							SDLProcessGameControllerButton(&OldController->ActionLeft,
								SDL_GameControllerGetButton(Controller, SDL_CONTROLLER_BUTTON_X),
								&NewController->ActionLeft);
							SDLProcessGameControllerButton(&OldController->ActionUp,
								SDL_GameControllerGetButton(Controller, SDL_CONTROLLER_BUTTON_Y),
								&NewController->ActionUp);
							SDLProcessGameControllerButton(&OldController->LeftShoulder,
								SDL_GameControllerGetButton(Controller, SDL_CONTROLLER_BUTTON_LEFTSHOULDER),
								&NewController->LeftShoulder);
							SDLProcessGameControllerButton(&OldController->RightShoulder,
								SDL_GameControllerGetButton(Controller, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER),
								&NewController->RightShoulder);

							SDLProcessGameControllerButton(&OldController->Start,
								SDL_GameControllerGetButton(Controller, SDL_CONTROLLER_BUTTON_START),
								&NewController->Start);
							SDLProcessGameControllerButton(&OldController->Back,
								SDL_GameControllerGetButton(Controller, SDL_CONTROLLER_BUTTON_BACK),
								&NewController->Back);

						}
						else
						{
							// NOTE(casey): The controller is not available
							NewController->IsConnected = false;
						}
					}

					game_offscreen_buffer Buffer = {};
					Buffer.Memory = GlobalBackbuffer.Memory;
					Buffer.Width = GlobalBackbuffer.Width;
					Buffer.Height = GlobalBackbuffer.Height;
					Buffer.Pitch = GlobalBackbuffer.Pitch;

					if (!SoundIsValid)
					{
						SoundIsValid = true;
					}

					SDL_LockAudio();
					int ByteToLock = (SoundOutput.RunningSampleIndex*SoundOutput.BytesPerSample) % SoundOutput.SecondaryBufferSize;
					int TargetCursor = ((GlobalSecondaryBuffer.PlayCursor + 
						(SoundOutput.SafetyBytes*SoundOutput.BytesPerSample)) % 
						SoundOutput.SecondaryBufferSize);
					int BytesToWrite;
					if (ByteToLock > TargetCursor)
					{
						BytesToWrite = (SoundOutput.SecondaryBufferSize - ByteToLock);
						BytesToWrite += TargetCursor;
					}
					else
					{
						BytesToWrite = TargetCursor - ByteToLock;
					}
					SDL_UnlockAudio();

					game_sound_output_buffer SoundBuffer = {};
					SoundBuffer.SamplesPerSecond = SoundOutput.SamplesPerSecond;
					SoundBuffer.SampleCount = Align8(BytesToWrite / SoundOutput.BytesPerSample);
					BytesToWrite = SoundBuffer.SampleCount*SoundOutput.BytesPerSample;
					SoundBuffer.Samples = Samples;


					GameUpdateAndRender(&GameMemory, NewInput, &Buffer, &SoundBuffer);
					SDLFillSoundBuffer(&SoundOutput, ByteToLock, BytesToWrite, &SoundBuffer);

					//TODO: Leave this off untill there is v support.
#if 0
					u64 WorkCounter = SDLGetWallClock();
					r32 WorkSecondsElapsed = SDLGetSecondsElapsed(LastCounter, WorkCounter);

					//TODO: Might be bugged
					r32 SecondsElapsedForFrame = WorkSecondsElapsed;
					if (SecondsElapsedForFrame < TargetSecondsPerFrame)
					{
						u32 SleepMS = (u32)(1000.0f * (TargetSecondsPerFrame - SecondsElapsedForFrame));

						if (SleepMS > 0)
						{
							SDL_Delay(SleepMS);
						}

						r32 TestSecondsElapsedForFrame = SDLGetSecondsElapsed(LastCounter, SDLGetWallClock());

						if (TestSecondsElapsedForFrame < TargetSecondsPerFrame)
						{
							//TODO: Missed sleep, logging
						}

						while (SecondsElapsedForFrame < TargetSecondsPerFrame)
						{
							SecondsElapsedForFrame = SDLGetSecondsElapsed(LastCounter, SDLGetWallClock());
						}
					}
					else
					{
						//TODO: Missed Frame, Logging
					}
#endif

					sdl_window_dimension Dimension = SDLGetWindowDimension(Window);
					SDLDisplayBufferInWindow(&GlobalBackbuffer, Renderer,
						Dimension.Width, Dimension.Height);

					FlipWallClock = SDLGetWallClock();

					game_input *Temp = NewInput;
					NewInput = OldInput;
					OldInput = Temp;

					u64 EndCounter = SDLGetWallClock();
					r32 MeasuredSecondsPerFrame = SDLGetSecondsElapsed(LastCounter, EndCounter);
					r32 ExactTargetFramesPerUpdate = MeasuredSecondsPerFrame * (r32)MonitorRefreshHz;
					u32 NewExpectedFramesPerUpdate = RoundReal32ToInt32(ExactTargetFramesPerUpdate);
					ExpectedFramesPerUpdate = NewExpectedFramesPerUpdate;

					TargetSecondsPerFrame = MeasuredSecondsPerFrame;
					
					LastCounter = EndCounter;
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

	SDLCloseGameControllers();
	SDL_Quit();

	return (0);
}