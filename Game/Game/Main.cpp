// Define to enable debugging functionality
#define OM_DEBUG 1

#include "Platform.h"
#include <windows.h>
#include <stdio.h>
#include <malloc.h>
#include <Xinput.h>
#include <dsound.h>

#include "Game_Intristics.h"
#include "Game_Math.h"

#include "Main.h" //TODO: Main.h should be renamed, same with Main.cpp.

/*
	TODO:
		* REWRITE TO BE A WINDOWS ONLY PLATFORM LAYER.
		* Saved game locations
		* Get handle to our own executable file
		* Asset loading path
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
om_global_variable b32 GlobalPause;
om_global_variable win32_offscreen_buffer GlobalBackbuffer;
om_global_variable LPDIRECTSOUNDBUFFER GlobalSecondaryBuffer;
om_global_variable u64 GlobalPerfCountFrequency;
om_global_variable b32 DEBUGGlobalShowCursor;
om_global_variable WINDOWPLACEMENT GlobalWindowPosition = { sizeof(GlobalWindowPosition) };

// NOTE: XInputGetState
#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE *pState)
typedef X_INPUT_GET_STATE(x_input_get_state);
X_INPUT_GET_STATE(XInputGetStateStub)
{
	return (ERROR_DEVICE_NOT_CONNECTED);
}
om_global_variable x_input_get_state *XInputGetState_ = XInputGetStateStub;
#define XInputGetState XInputGetState_

// NOTE: XInputSetState
#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION *pVibration)
typedef X_INPUT_SET_STATE(x_input_set_state);
X_INPUT_SET_STATE(XInputSetStateStub)
{
	return (ERROR_DEVICE_NOT_CONNECTED);
}
om_global_variable x_input_set_state *XInputSetState_ = XInputSetStateStub;
#define XInputSetState XInputSetState_

#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter)
typedef DIRECT_SOUND_CREATE(direct_sound_create);

om_internal void
ConcatStrings(char *SourceA, size_t SourceACount, char *SourceB, size_t SourceBCount,
	char *Destination, size_t DestinationCount)
{
	for (u32 Index = 0; Index < SourceACount; ++Index)
	{
		*Destination++ = *SourceA++;
	}

	for (u32 Index = 0; Index < SourceBCount; ++Index)
	{
		*Destination++ = *SourceB++;
	}

	*Destination++ = 0;
}

om_internal void
Win32GetEXEFileName(win32_state *State)
{
	// NOTE: Don't use MAX_PATH in user-facing code since it can lead for wrong results.
	// This is for debug code only.
	DWORD SizeOfFileName = GetModuleFileNameA(0, State->EXEFileName, sizeof(State->EXEFileName));
	State->OnePastLastEXEFileNameSlash = State->EXEFileName;
	for (char *Scan = State->EXEFileName; *Scan; ++Scan)
	{
		if (*Scan == '\\')
		{
			State->OnePastLastEXEFileNameSlash = Scan + 1;
		}
	}
}

om_internal int
StringLength(char *String)
{
	int Count = 0;
	while (*String++)
	{
		++Count;
	}
	return (Count);
}

om_internal void
Win32BuildEXEPathFileName(win32_state *State, char *FileName, char *Destination, int DestinationCount)
{
	ConcatStrings(State->EXEFileName, State->OnePastLastEXEFileNameSlash - State->EXEFileName,
		FileName, StringLength(FileName),
		Destination, DestinationCount);
}

DEBUG_PLATFORM_FREE_FILE_MEMORY(DEBUGPlatformFreeFileMemory)
{
	if (Memory)
	{
		VirtualFree(Memory, NULL, MEM_RELEASE);
	}
}

DEBUG_PLATFORM_READ_ENTIRE_FILE(DEBUGPlatformReadEntireFile)
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

DEBUG_PLATFORM_WRITE_ENTIRE_FILE(DEBUGPlatformWriteEntireFile)
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

struct win32_platform_file_handle
{
	platform_file_handle Handle;
	HANDLE Win32Handle;
};

struct win32_platform_file_group
{
	platform_file_group Group;
	HANDLE FindHandle;
	WIN32_FIND_DATAA FindData;
};

om_internal PLATFORM_GET_ALL_FILE_OF_TYPE_BEGIN(Win32GetAllFilesOfTypeBegin)
{
	win32_platform_file_group *Win32FileGroup = (win32_platform_file_group *)VirtualAlloc(
		0, sizeof(win32_platform_file_group), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

	char *TypeAt = Type;
	char WildCard[32] = "*.";
	
	// NOTE: Copying filetype to the end of wildcard.
	for (u32 WildCardIndex = 2; WildCardIndex < sizeof(WildCard); ++WildCardIndex)
	{
		WildCard[WildCardIndex] = *TypeAt;
		if (*TypeAt == 0)
		{
			break;
		}

		++TypeAt;
	}
	WildCard[sizeof(WildCard) - 1] = 0;

	Win32FileGroup->Group.FileCount = 0;

	WIN32_FIND_DATAA FindData;
	HANDLE FindHandle = FindFirstFileA(WildCard, &FindData);
	while (FindHandle != INVALID_HANDLE_VALUE)
	{
		++Win32FileGroup->Group.FileCount;

		if (!FindNextFileA(FindHandle, &FindData))
		{
			break;
		}
	}
	FindClose(FindHandle);

	Win32FileGroup->FindHandle = FindFirstFileA(WildCard, &Win32FileGroup->FindData);

	return ((platform_file_group *)Win32FileGroup);
}

om_internal PLATFORM_GET_ALL_FILE_OF_TYPE_END(Win32GetAllFilesOfTypeEnd)
{
	win32_platform_file_group *Win32FileGroup = (win32_platform_file_group *)FileGroup;

	if (Win32FileGroup)
	{
		FindClose(Win32FileGroup->FindHandle);

		VirtualFree(Win32FileGroup, 0, MEM_RELEASE);
	}
}

om_internal PLATFORM_FILE_ERROR(Win32FileError)
{
#if 1
	OutputDebugString("WIN32 FILE ERROR: ");
	OutputDebugString(Message);
	OutputDebugString("\n");
#endif

	Handle->NoErrors = false;

	//CloseHandle(FileHandle);
}

om_internal PLATFORM_OPEN_FILE(Win32OpenNextFile)
{
	
	win32_platform_file_group *Win32FileGroup = (win32_platform_file_group *)FileGroup;
	win32_platform_file_handle *Result = 0;

	
	if (Win32FileGroup->FindHandle != INVALID_HANDLE_VALUE)
	{
		Result = (win32_platform_file_handle *)VirtualAlloc(0, sizeof(win32_platform_file_handle), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
		
		if (Result)
		{
			char *FileName = Win32FileGroup->FindData.cFileName;
			Result->Win32Handle = CreateFileA(FileName, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
			Result->Handle.NoErrors = (Result->Win32Handle != INVALID_HANDLE_VALUE);
		}

		if (!FindNextFileA(Win32FileGroup->FindHandle, &Win32FileGroup->FindData))
		{
			FindClose(Win32FileGroup->FindHandle);
			Win32FileGroup->FindHandle = INVALID_HANDLE_VALUE;
		}
	}

	return ((platform_file_handle *)Result);
}

om_internal PLATFORM_READ_DATA_FROM_FILE(Win32ReadDataFromFile)
{
	if (PlatformNoFileErrors(Source))
	{
		win32_platform_file_handle *Handle = (win32_platform_file_handle *)Source;
		OVERLAPPED Overlapped = {};
		Overlapped.Offset = (u32)((Offset >> 0) & 0xFFFFFFFF);
		Overlapped.OffsetHigh = (u32)((Offset >> 32) & 0xFFFFFFFF);

		u32 FileSize32 = SafeTruncateUInt64(Size);

		DWORD BytesRead;
		if (ReadFile(Handle->Win32Handle, Destination, FileSize32, &BytesRead, &Overlapped) && (FileSize32 == BytesRead))
		{
			// NOTE: File read success.
		}
		else
		{
			Win32FileError(&Handle->Handle, "Read file failed.");
		}
	}
}

inline FILETIME
Win32GetLastWriteTime(const char* FileName)
{
	FILETIME Result = {};

	WIN32_FILE_ATTRIBUTE_DATA Data;
	if (GetFileAttributesEx(FileName, GetFileExInfoStandard, &Data))
	{
		Result = Data.ftLastWriteTime;
	}

	return (Result);
}

om_internal win32_game_code
Win32LoadGameCode(const char* SourceDLLName, const char* TempDLLName, char *LockFileName)
{
	win32_game_code Result = {};
	
	WIN32_FILE_ATTRIBUTE_DATA Ignored;
	if (!GetFileAttributesEx(LockFileName, GetFileExInfoStandard, &Ignored))
	{
		Result.DLLLastWriteTime = Win32GetLastWriteTime(SourceDLLName);

		CopyFile(SourceDLLName, TempDLLName, FALSE);
		
		Result.GameCodeDLL = LoadLibraryA(TempDLLName);
		if (Result.GameCodeDLL)
		{
			Result.UpdateAndRender = (game_update_and_render *)GetProcAddress(Result.GameCodeDLL, "GameUpdateAndRender");
			Result.GetSoundSamples = (game_get_sound_samples *)GetProcAddress(Result.GameCodeDLL, "GameGetSoundSamples");

			Result.IsValid = (Result.UpdateAndRender && Result.GetSoundSamples);
		}
	}

	if (!Result.IsValid)
	{
		Result.UpdateAndRender = 0;
		Result.GetSoundSamples = 0;
	}

	return (Result);
}

om_internal void
Win32UnloadGameCode(win32_game_code *GameCode)
{
	if (GameCode->GameCodeDLL)
	{
		FreeLibrary(GameCode->GameCodeDLL);
		GameCode->GameCodeDLL = 0;
	}

	GameCode->IsValid = false;
	GameCode->UpdateAndRender = 0;
	GameCode->GetSoundSamples = 0;
}

om_internal void
Win32LoadXInput(void)
{
	// TODO: Test this on Windows 8
	HMODULE XInputLibrary = LoadLibraryA("xinput1_4.dll");
	if (!XInputLibrary)
	{
		//TODO: Diagnostic
		XInputLibrary = LoadLibraryA("xinput9_1_0.dll");
	}

	if (!XInputLibrary)
	{
		//TODO: Diagnostic
		XInputLibrary = LoadLibraryA("xinput1_3.dll");
	}

	if (XInputLibrary)
	{
		XInputGetState = (x_input_get_state *)GetProcAddress(XInputLibrary, "XInputGetState");
		if (!XInputGetState) { XInputGetState = XInputGetStateStub; }

		XInputSetState = (x_input_set_state *)GetProcAddress(XInputLibrary, "XInputSetState");
		if (!XInputSetState) { XInputSetState = XInputSetStateStub; }

		//TODO: Diagnostic
	}
	else
	{
		//TODO: Diagnostic
	}
}

om_internal void
Win32InitDSound(HWND Window, i32 SamplesPerSecond, i32 BufferSize)
{
	//NOTE: Loading the library
	HMODULE DSoundLibrary = LoadLibraryA("dsound.dll");
	if (DSoundLibrary)
	{
		//NOTE: Getting a DirectSound object.
		direct_sound_create *DirectSoundCreate = (direct_sound_create *)GetProcAddress(DSoundLibrary, "DirectSoundCreate");

		LPDIRECTSOUND DirectSound;
		if (DirectSoundCreate && SUCCEEDED(DirectSoundCreate(0, &DirectSound, 0)))
		{
			WAVEFORMATEX WaveFormat = {};
			WaveFormat.wFormatTag = WAVE_FORMAT_PCM;
			WaveFormat.nChannels = 2;
			WaveFormat.nSamplesPerSec = SamplesPerSecond;
			WaveFormat.wBitsPerSample = 16;
			WaveFormat.nBlockAlign = (WaveFormat.nChannels*WaveFormat.wBitsPerSample) / 8;
			WaveFormat.nAvgBytesPerSec = WaveFormat.nSamplesPerSec*WaveFormat.nBlockAlign;
			WaveFormat.cbSize = 0;

			if (SUCCEEDED(DirectSound->SetCooperativeLevel(Window, DSSCL_PRIORITY)))
			{
				DSBUFFERDESC BufferDescription = {};
				BufferDescription.dwSize = sizeof(BufferDescription);
				BufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;

				// NOTE: Creating a primary buffer
				LPDIRECTSOUNDBUFFER PrimaryBuffer;
				if (SUCCEEDED(DirectSound->CreateSoundBuffer(&BufferDescription, &PrimaryBuffer, 0)))
				{
					HRESULT Error = PrimaryBuffer->SetFormat(&WaveFormat);
					if (SUCCEEDED(Error))
					{
						// NOTE: We've set the format here.
						OutputDebugStringA("Primary buffer format has been set.\n");
					}
					else
					{
						//TODO: Diagnostic
					}
				}
				else
				{
					//TODO: Diagnostic
				}
			}
			else
			{
				//TODO: Diagnostic
			}

			DSBUFFERDESC BufferDescription = {};
			BufferDescription.dwSize = sizeof(BufferDescription);
			BufferDescription.dwFlags = DSBCAPS_GETCURRENTPOSITION2;
			//BufferDescription.dwFlags |= DSBCAPS_GLOBALFOCUS;
			BufferDescription.dwBufferBytes = BufferSize;
			BufferDescription.lpwfxFormat = &WaveFormat;
			HRESULT Error = DirectSound->CreateSoundBuffer(&BufferDescription, &GlobalSecondaryBuffer, 0);
			if (SUCCEEDED(Error))
			{
				OutputDebugStringA("Secondary buffer created successfully.\n");
			}
		}
		else
		{
			// TODO: Diagnostic
		}
	}
	else
	{
		// TODO: Diagnostic
	}
}

om_internal win32_window_dimension
Win32GetWindowDimension(HWND Window)
{
	win32_window_dimension Result;

	RECT ClientRect;
	GetClientRect(Window, &ClientRect);
	Result.Width = ClientRect.right - ClientRect.left;
	Result.Height = ClientRect.bottom - ClientRect.top;

	return (Result);
}

om_internal void
Win32ResizeDIBSection(win32_offscreen_buffer *Buffer, int Width, int Height)
{
	if (Buffer->Memory)
	{
		VirtualFree(Buffer->Memory, 0, MEM_RELEASE);
	}

	Buffer->Width = Width;
	Buffer->Height = Height;

	int BytesPerPixel = 4;
	Buffer->BytesPerPixel = BytesPerPixel;

	// NOTE: When the biHeight field is negative, this is the clue to
	// Windows to treat this bitmap as top-down, not bottom-up, meaning that
	// the first three bytes of the image are the color for the top left pixel
	// in the bitmap, not the bottom left!
	Buffer->Info.bmiHeader.biSize = sizeof(Buffer->Info.bmiHeader);
	Buffer->Info.bmiHeader.biWidth = Buffer->Width;
	Buffer->Info.bmiHeader.biHeight = -Buffer->Height;
	Buffer->Info.bmiHeader.biPlanes = 1;
	Buffer->Info.bmiHeader.biBitCount = 32;
	Buffer->Info.bmiHeader.biCompression = BI_RGB;


	Buffer->Pitch = Align16(Width*BytesPerPixel);
	int BitmapMemorySize = (Buffer->Pitch*Buffer->Height);
	Buffer->Memory = VirtualAlloc(0, BitmapMemorySize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

	//TODO: Prolly clear to black.
}

om_internal void
Win32DisplayBufferInWindow(win32_offscreen_buffer *Buffer, HDC DeviceContext, int WindowWidth, int WindowHeight)
{
	if ((WindowWidth >= Buffer->Width * 2) && (WindowHeight >= Buffer->Height * 2))
	{
		StretchDIBits(DeviceContext, 
			0, 0, 2 * Buffer->Width, 2 * Buffer->Height, 
			0, 0, Buffer->Width, Buffer->Height, 
			Buffer->Memory, 
			&Buffer->Info, 
			DIB_RGB_COLORS, SRCCOPY);
	}
	else
	{
		int OffsetX = 0;
		int OffsetY = 0;

		// NOTE: For prototyping purposes, we're going to always blit
		// 1-to-1 pixels to make sure we don't introduce artifacts with
		// stretching while we are learning to code the renderer!
		StretchDIBits(DeviceContext,
			OffsetX, OffsetY, Buffer->Width, Buffer->Height,
			0, 0, Buffer->Width, Buffer->Height,
			Buffer->Memory,
			&Buffer->Info,
			DIB_RGB_COLORS, SRCCOPY);
	}
}

om_internal LRESULT CALLBACK
Win32MainWindowCallback(HWND Window, UINT Message, WPARAM WParam, LPARAM LParam)
{
	LRESULT Result = 0;

	switch (Message)
	{
		case WM_CLOSE:
		{
			// TODO: Handle this with message to user?
			GlobalRunning = false;
		} break;

		case WM_SETCURSOR:
		{
			if (DEBUGGlobalShowCursor)
			{
				Result = DefWindowProcA(Window, Message, WParam, LParam);
			}
			else
			{
				SetCursor(0);
			}
		} break;

		case WM_ACTIVATEAPP:
		{
		} break;

		case WM_DESTROY:
		{
			// TODO: Handle this as error? - Recreate the window?
			GlobalRunning = false;
		} break;

		case WM_SYSKEYDOWN:
		case WM_SYSKEYUP:
		case WM_KEYDOWN:
		case WM_KEYUP:
		{
			OM_ASSERT(!"Keyboard input came through a non-dispatch message");
		} break;

		case WM_PAINT:
		{
			PAINTSTRUCT Paint;
			HDC DeviceContext = BeginPaint(Window, &Paint);
			win32_window_dimension Dimension = Win32GetWindowDimension(Window);
			Win32DisplayBufferInWindow(&GlobalBackbuffer, DeviceContext, Dimension.Width, Dimension.Height);
			EndPaint(Window, &Paint);
		} break;

		default:
		{
			Result = DefWindowProcA(Window, Message, WParam, LParam);
		} break;
	}

	return (Result);
}

om_internal void
Win32ClearBuffer(win32_sound_output *SoundOutput)
{
	VOID *Region1;
	DWORD Region1Size;
	VOID *Region2;
	DWORD Region2Size;

	if (SUCCEEDED(GlobalSecondaryBuffer->Lock(0, SoundOutput->SecondaryBufferSize,
		&Region1, &Region1Size,
		&Region2, &Region2Size,
		0)))
	{
		//TODO: Assert that Region1Size / Region2Size is valid.
		u8 *DestSample = (u8 *)Region1;
		for (DWORD ByteIndex = 0; ByteIndex < Region1Size; ++ByteIndex)
		{
			*DestSample++ = 0;
		}

		DestSample = (u8 *)Region2;
		for (DWORD ByteIndex = 0; ByteIndex < Region2Size; ++ByteIndex)
		{
			*DestSample++ = 0;
		}

		GlobalSecondaryBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);
	}
}

om_internal void
Win32FillSoundBuffer(win32_sound_output *SoundOutput, DWORD ByteToLock, DWORD BytesToWrite, game_sound_output_buffer *SourceBuffer)
{
	//TODO: More strenous test.
	VOID *Region1;
	DWORD Region1Size;
	VOID *Region2;
	DWORD Region2Size;

	if (SUCCEEDED(GlobalSecondaryBuffer->Lock(ByteToLock, BytesToWrite,
		&Region1, &Region1Size,
		&Region2, &Region2Size,
		0)))
	{
		//TODO: Assert that Region1Size / Region2Size is valid.

		//TODO: Collapse the two loops.
		DWORD Region1SampleCount = Region1Size / SoundOutput->BytesPerSample;
		i16 *DestSample = (i16 *)Region1;
		i16 *SourceSample = SourceBuffer->Samples;
		for (DWORD SampleIndex = 0; SampleIndex < Region1SampleCount; ++SampleIndex)
		{
			*DestSample++ = *SourceSample++;
			*DestSample++ = *SourceSample++;
			++SoundOutput->RunningSampleIndex;
		}

		DWORD Region2SampleCount = Region2Size / SoundOutput->BytesPerSample;
		DestSample = (i16 *)Region2;
		for (DWORD SampleIndex = 0; SampleIndex < Region2SampleCount; ++SampleIndex)
		{
			*DestSample++ = *SourceSample++;
			*DestSample++ = *SourceSample++;
			++SoundOutput->RunningSampleIndex;
		}

		GlobalSecondaryBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);
	}
}

om_internal void
Win32ProcessKeyboardMessage(game_button_state *NewState, b32 IsDown)
{
	if (NewState->EndedDown != IsDown)
	{
		NewState->EndedDown = IsDown;
		++NewState->HalfTransitionCount;
	}
}

om_internal void
Win32ProcessXInputDigitalButton(DWORD XInputButtonState, game_button_state *OldState, DWORD ButtonBit, 
	game_button_state *NewState)
{
	NewState->EndedDown = ((XInputButtonState & ButtonBit) == ButtonBit);
	NewState->HalfTransitionCount = (OldState->EndedDown != NewState->EndedDown) ? 1 : 0;
}

om_internal r32
Win32ProcessXInputStickValue(SHORT Value, SHORT DeadZoneThreshold)
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

	return(Result);
}

om_internal void
Win32GetInputFileLocation(win32_state *State, b32 InputStream,
	int SlotIndex, int DestCount, char *Dest)
{
	char Temp[64];
	wsprintf(Temp, "loop_edit_%d_%s.hmi", SlotIndex, InputStream ? "input" : "state");
	Win32BuildEXEPathFileName(State, Temp, Dest, DestCount);
}

om_internal win32_replay_buffer *
Win32GetReplayBuffer(win32_state *State, int unsigned Index)
{
	OM_ASSERT(Index > 0);
	OM_ASSERT(Index < OM_ARRAYCOUNT(State->ReplayBuffers));
	win32_replay_buffer *Result = &State->ReplayBuffers[Index];
	return(Result);
}

om_internal void
Win32BeginRecordingInput(win32_state *State, int InputRecordingIndex)
{
	win32_replay_buffer *ReplayBuffer = Win32GetReplayBuffer(State, InputRecordingIndex);
	if (ReplayBuffer->MemoryBlock)
	{
		State->InputRecordingIndex = InputRecordingIndex;

		char FileName[WIN32_STATE_FILE_NAME_COUNT];
		Win32GetInputFileLocation(State, true, InputRecordingIndex, sizeof(FileName), FileName);
		State->RecordingHandle = CreateFileA(FileName, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);

#if 0
		LARGE_INTEGER FilePosition;
		FilePosition.QuadPart = State->TotalSize;
		SetFilePointerEx(State->RecordingHandle, FilePosition, 0, FILE_BEGIN);
#endif

		CopyMemory(ReplayBuffer->MemoryBlock, State->GameMemoryBlock, State->TotalSize);
	}
}

om_internal void
Win32EndRecordingInput(win32_state *State)
{
	CloseHandle(State->RecordingHandle);
	State->InputRecordingIndex = 0;
}

om_internal void
Win32BeginInputPlayBack(win32_state *State, int InputPlayingIndex)
{
	win32_replay_buffer *ReplayBuffer = Win32GetReplayBuffer(State, InputPlayingIndex);
	if (ReplayBuffer->MemoryBlock)
	{
		State->InputPlayingIndex = InputPlayingIndex;

		char FileName[WIN32_STATE_FILE_NAME_COUNT];
		Win32GetInputFileLocation(State, true, InputPlayingIndex, sizeof(FileName), FileName);
		State->PlaybackHandle = CreateFileA(FileName, GENERIC_READ, 0, 0, OPEN_EXISTING, 0, 0);

#if 0
		LARGE_INTEGER FilePosition;
		FilePosition.QuadPart = State->TotalSize;
		SetFilePointerEx(State->PlaybackHandle, FilePosition, 0, FILE_BEGIN);
#endif

		CopyMemory(State->GameMemoryBlock, ReplayBuffer->MemoryBlock, State->TotalSize);
	}
}

om_internal void
Win32EndInputPlayBack(win32_state *State)
{
	CloseHandle(State->PlaybackHandle);
	State->InputPlayingIndex = 0;
}

om_internal void
Win32RecordInput(win32_state *State, game_input *NewInput)
{
	DWORD BytesWritten;
	WriteFile(State->RecordingHandle, NewInput, sizeof(*NewInput), &BytesWritten, 0);
}

om_internal void
Win32PlayBackInput(win32_state *State, game_input *NewInput)
{
	DWORD BytesRead = 0;
	if (ReadFile(State->PlaybackHandle, NewInput, sizeof(*NewInput), &BytesRead, 0))
	{
		if (BytesRead == 0)
		{
			// NOTE: We've hit the end of the stream, go back to the beginning
			int PlayingIndex = State->InputPlayingIndex;
			Win32EndInputPlayBack(State);
			Win32BeginInputPlayBack(State, PlayingIndex);
			ReadFile(State->PlaybackHandle, NewInput, sizeof(*NewInput), &BytesRead, 0);
		}
	}
}

om_internal void
ToggleFullscreen(HWND Window)
{
	// NOTE: This is following the Raymond Chen prescription for fullscreen toggling,
	// see: http://blogs.msdn.com/b/oldnewthing/archive/2010/04/12/9994016.aspx

	DWORD Style = GetWindowLong(Window, GWL_STYLE);
	if (Style & WS_OVERLAPPEDWINDOW)
	{
		MONITORINFO MonitorInfo = { sizeof(MonitorInfo) };
		if (GetWindowPlacement(Window, &GlobalWindowPosition) &&
			GetMonitorInfo(MonitorFromWindow(Window, MONITOR_DEFAULTTOPRIMARY), &MonitorInfo))
		{
			SetWindowLong(Window, GWL_STYLE, Style & WS_OVERLAPPED);
			SetWindowPos(Window, HWND_TOP,
				MonitorInfo.rcMonitor.left, MonitorInfo.rcMonitor.top,
				MonitorInfo.rcMonitor.right - MonitorInfo.rcMonitor.left,
				MonitorInfo.rcMonitor.bottom - MonitorInfo.rcMonitor.top,
				SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
		}
	}
	else
	{
		SetWindowLong(Window, GWL_STYLE, Style | WS_OVERLAPPEDWINDOW);
		SetWindowPlacement(Window, &GlobalWindowPosition);
		SetWindowPos(Window, 0, 0, 0, 0, 0,
			SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
			SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
	}
}

om_internal void
Win32ProcessPendingMessages(win32_state *State, game_controller_input *KeyboardController)
{
	MSG Message;
	while (PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
	{
		switch (Message.message)
		{
			case WM_QUIT:
			{
				GlobalRunning = false;
			} break;

			case WM_SYSKEYDOWN:
			case WM_SYSKEYUP:
			case WM_KEYDOWN:
			case WM_KEYUP:
			{
				u32 VKCode = (u32)Message.wParam;

				// NOTE: Since we are comparing WasDown to IsDown
				// we mush use == and != to convert these bit tests to actual 0 or 1 values.
				b32 WasDown = ((Message.lParam & (1 << 30)) != 0);
				b32 IsDown = ((Message.lParam & (1 << 31)) == 0);

				if (WasDown != IsDown)
				{
					if (VKCode == 'W')
					{
						Win32ProcessKeyboardMessage(&KeyboardController->MoveUp, IsDown);
					}
					else if (VKCode == 'A')
					{
						Win32ProcessKeyboardMessage(&KeyboardController->MoveLeft, IsDown);
					}
					else if (VKCode == 'S')
					{
						Win32ProcessKeyboardMessage(&KeyboardController->MoveDown, IsDown);
					}
					else if (VKCode == 'D')
					{
						Win32ProcessKeyboardMessage(&KeyboardController->MoveRight, IsDown);
					}
					else if (VKCode == 'Q')
					{
						Win32ProcessKeyboardMessage(&KeyboardController->LeftShoulder, IsDown);
					}
					else if (VKCode == 'E')
					{
						Win32ProcessKeyboardMessage(&KeyboardController->RightShoulder, IsDown);
					}
					else if (VKCode == VK_UP)
					{
						Win32ProcessKeyboardMessage(&KeyboardController->ActionUp, IsDown);
					}
					else if (VKCode == VK_LEFT)
					{
						Win32ProcessKeyboardMessage(&KeyboardController->ActionLeft, IsDown);
					}
					else if (VKCode == VK_DOWN)
					{
						Win32ProcessKeyboardMessage(&KeyboardController->ActionDown, IsDown);
					}
					else if (VKCode == VK_RIGHT)
					{
						Win32ProcessKeyboardMessage(&KeyboardController->ActionRight, IsDown);
					}
					else if (VKCode == VK_ESCAPE)
					{
						Win32ProcessKeyboardMessage(&KeyboardController->Back, IsDown);
					}
					else if (VKCode == VK_SPACE)
					{
						Win32ProcessKeyboardMessage(&KeyboardController->Start, IsDown);
					}
					// Internal testing start
					else if (VKCode == 'P')
					{
						if (IsDown)
						{
							GlobalPause = !GlobalPause;
						}
					}
					else if (VKCode == 'L')
					{
						if (IsDown)
						{
							if (State->InputPlayingIndex == 0)
							{
								if (State->InputRecordingIndex == 0)
								{
									Win32BeginRecordingInput(State, 1);
								}
								else
								{
									Win32EndRecordingInput(State);
									Win32BeginInputPlayBack(State, 1);
								}
							}
							else
							{
								Win32EndInputPlayBack(State);
							}
						}
					}
					// Internal testing end

					if (IsDown)
					{
						b32 AltKeyWasDown = (Message.lParam & (1 << 29));
						if ((VKCode == VK_F4) && AltKeyWasDown)
						{
							GlobalRunning = false;
						}
						if ((VKCode == VK_RETURN) && AltKeyWasDown)
						{
							if (Message.hwnd)
							{
								ToggleFullscreen(Message.hwnd);
							}
						}
					}
				}

			} break;

			default:
			{
				TranslateMessage(&Message);
				DispatchMessage(&Message);
			} break;
		}
	}
}

inline LARGE_INTEGER
Win32GetWallClock(void)
{
	LARGE_INTEGER Result;

	QueryPerformanceCounter(&Result);

	return (Result);
}

inline r32
Win32GetSecondsElapsed(LARGE_INTEGER Start, LARGE_INTEGER End)
{
	r32 Result = ((r32)(End.QuadPart - Start.QuadPart) / (r32)GlobalPerfCountFrequency);

	return (Result);
}

om_internal void
HandleDebugCycleCounters(game_memory *Memory)
{
#if 1 // TODO: Add actual define to use for enabling / Disabling this.
	OutputDebugStringA("DEBUG CYCLE COUNTS: \n");
	for (int CounterIndex = 0; CounterIndex < OM_ARRAYCOUNT(Memory->Counters); ++CounterIndex)
	{
		debug_cycle_counter *Counter = Memory->Counters + CounterIndex;

		if (Counter->HitCount)
		{
			// NOTE: This will output the timers for the functions marked to be timed.
			// The different parts of the output are:
			// 1. Counter Index (The function we are timing)
			// 2. 64 bit cycle count, this includes things windows does in the background.
			// 3. How many times the function was hit.
			// 4. How many cycles on average per hit.

			char TextBuffer[256];
			// TODO: Make this easier to read.
			_snprintf_s(TextBuffer, sizeof(TextBuffer),
				"  %d: %I64ucy %uh %I64ucy/h\n",
				CounterIndex,
				Counter->CycleCount,
				Counter->HitCount,
				Counter->CycleCount / Counter->HitCount);
			OutputDebugStringA(TextBuffer);
			Counter->HitCount = 0;
			Counter->CycleCount = 0;
		}
	}
#endif
}

om_internal void
HandleDebugTimeCounters(game_memory *Memory)
{
#if 1
	OutputDebugStringA("DEBUG TIME COUNTS: \n");
	for (u32 TimerIndex = 0; TimerIndex < OM_ARRAYCOUNT(Memory->TimeCounters); ++TimerIndex)
	{
		debug_time_counter *Counter = Memory->TimeCounters + TimerIndex;

		char TextBuffer[256];
		_snprintf_s(TextBuffer, sizeof(TextBuffer), "%d: %I64ums\n", TimerIndex, Counter->Time);
		OutputDebugStringA(TextBuffer);
		Counter->Time = 0;
	}
#endif
}

struct platform_thread_queue_entry
{
	platform_thread_queue_callback *Callback;
	void *Data;
};
struct platform_thread_queue
{
	u32 volatile CompletionGoal;
	u32 volatile CompletionCount;

	u32 volatile NextEntryToWrite;
	u32 volatile NextEntryToRead;

	HANDLE SemaphoreHandle;

	platform_thread_queue_entry Entries[256];
};

struct win32_thread_info
{
	int LogicalThreadIndex;
	platform_thread_queue *Queue;
};

om_internal void
Win32AddThreadQueueEntry(platform_thread_queue *Queue, platform_thread_queue_callback *Callback, void *Data)
{
	u32 NewNextEntryToWrite = (Queue->NextEntryToWrite + 1) % OM_ARRAYCOUNT(Queue->Entries);
	OM_ASSERT(NewNextEntryToWrite != Queue->NextEntryToRead);

	platform_thread_queue_entry *Entry = Queue->Entries + Queue->NextEntryToWrite;
	Entry->Callback = Callback;
	Entry->Data = Data;
	++Queue->CompletionGoal;

	_WriteBarrier();

	Queue->NextEntryToWrite = NewNextEntryToWrite;
	
	ReleaseSemaphore(Queue->SemaphoreHandle, 1, 0);
}

om_internal b32
Win32DoNextThreadQueueEntry(platform_thread_queue *Queue)
{
	b32 ShouldSleep = false;

	u32 OriginalNextEntryToRead = Queue->NextEntryToRead;
	u32 NewNextEntryToRead = (OriginalNextEntryToRead + 1) % OM_ARRAYCOUNT(Queue->Entries);

	if (OriginalNextEntryToRead != Queue->NextEntryToWrite)
	{
		u32 Index = InterlockedCompareExchange((LONG volatile *)&Queue->NextEntryToRead, NewNextEntryToRead, OriginalNextEntryToRead);

		if (Index == OriginalNextEntryToRead)
		{
			platform_thread_queue_entry Entry = Queue->Entries[Index];
			Entry.Callback(Queue, Entry.Data);
			InterlockedIncrement((LONG volatile *)&Queue->CompletionCount);
		}
	}
	else
	{
		ShouldSleep = true;
	}

	return (ShouldSleep);
}

om_internal void
Win32CompleteAllThreadWork(platform_thread_queue *Queue)
{
	while (Queue->CompletionGoal != Queue->CompletionCount)
	{
		Win32DoNextThreadQueueEntry(Queue);
	}

	Queue->CompletionGoal = 0;
	Queue->CompletionCount = 0;
}

om_internal 
PLATFORM_THREAD_QUEUE_CALLBACK(DoThreadQueueWork)
{
	char Buffer[256];
	wsprintf(Buffer, "Thread %u: %s\n", GetCurrentThreadId, (char *)Data);
	OutputDebugStringA(Buffer);
}

DWORD WINAPI
ThreadProc(LPVOID lpParameter)
{
	platform_thread_queue *Queue = (platform_thread_queue *)lpParameter;

	for (;;)
	{
		if (Win32DoNextThreadQueueEntry(Queue))
		{
			WaitForSingleObjectEx(Queue->SemaphoreHandle, INFINITE, FALSE);
		}
	}

//	return (0);
}

om_internal void
Win32MakeThreadQueue(platform_thread_queue *Queue, u32 ThreadCount)
{
	Queue->CompletionGoal = 0;
	Queue->CompletionCount = 0;

	Queue->NextEntryToWrite = 0;
	Queue->NextEntryToRead = 0;

	u32 InitialCount = 0;
	Queue->SemaphoreHandle = CreateSemaphoreEx(0, InitialCount, ThreadCount, 0, 0, SEMAPHORE_ALL_ACCESS);

	for (u32 ThreadIndex = 0; ThreadIndex < ThreadCount; ++ThreadIndex)
	{
		DWORD ThreadID;
		HANDLE ThreadHandle = CreateThread(0, 0, ThreadProc, Queue, 0, &ThreadID);
		CloseHandle(ThreadHandle);
	}
}

int CALLBACK
WinMain(HINSTANCE Instance, HINSTANCE PrevInstance, LPSTR CommandLine, int ShowCode)
{
	win32_state Win32State = {};

	platform_thread_queue HighPriorityQueue = {};
	Win32MakeThreadQueue(&HighPriorityQueue, 6);

	platform_thread_queue LowPriorityQueue = {};
	Win32MakeThreadQueue(&LowPriorityQueue, 2);

	LARGE_INTEGER PerfCountFrequencyResult;
	QueryPerformanceFrequency(&PerfCountFrequencyResult);
	GlobalPerfCountFrequency = PerfCountFrequencyResult.QuadPart;

	Win32GetEXEFileName(&Win32State);

	char SourceGameCodeDLLFullPath[WIN32_STATE_FILE_NAME_COUNT];
	Win32BuildEXEPathFileName(&Win32State, "GameCode.dll", 
		SourceGameCodeDLLFullPath, sizeof(SourceGameCodeDLLFullPath));
	
	char TempGameCodeDLLFullPath[WIN32_STATE_FILE_NAME_COUNT];
	Win32BuildEXEPathFileName(&Win32State, "GameCode_temp.dll", 
		TempGameCodeDLLFullPath, sizeof(TempGameCodeDLLFullPath));

	char GameCodeLockedFullPath[WIN32_STATE_FILE_NAME_COUNT];
	Win32BuildEXEPathFileName(&Win32State, "lock.tmp",
		GameCodeLockedFullPath, sizeof(GameCodeLockedFullPath));

	// NOTE: Set windows scheduler granularity to 1ms so that Sleep() can be more granular.
	UINT DesiredSchedulerMS = 1;
	b32 SleepIsGranular(timeBeginPeriod(DesiredSchedulerMS) == TIMERR_NOERROR);

	Win32LoadXInput();

	// NOTE: Toggle debug cursor.
	DEBUGGlobalShowCursor = true;

	WNDCLASSA WindowClass = {};

	/* NOTE: 1080p display mode is 1920x1080 -> Half of that is 960x540
	   1920 -> 2048 = 2048-1920 -> 128 pixels
	   1080 -> 2048 = 2048-1080 -> pixels 968
	   1024 + 128 = 1152
	*/
	//Win32ResizeDIBSection(&GlobalBackbuffer, 960, 540);
	//Win32ResizeDIBSection(&GlobalBackbuffer, 1920, 1080);
	//Win32ResizeDIBSection(&GlobalBackbuffer, 1279, 719);
	Win32ResizeDIBSection(&GlobalBackbuffer, 1280, 720);

	WindowClass.style = CS_HREDRAW | CS_VREDRAW;
	WindowClass.lpfnWndProc = Win32MainWindowCallback;
	WindowClass.hInstance = Instance;
	WindowClass.hCursor = LoadCursor(0, IDC_ARROW);
	// WindowClass.hIcon;
	WindowClass.lpszClassName = "GameWindowClass";

	if (RegisterClassA(&WindowClass))
	{
		HWND Window =
			CreateWindowExA(
				0,
				WindowClass.lpszClassName,
				"Game",
				WS_OVERLAPPEDWINDOW | WS_VISIBLE,
				CW_USEDEFAULT,
				CW_USEDEFAULT,
				CW_USEDEFAULT,
				CW_USEDEFAULT,
				0,
				0,
				Instance,
				0);

		if (Window)
		{
			win32_sound_output SoundOutput = {};

			// TODO: How to reliabely query this on windows?
			int MonitorRefreshHz = 60;
			HDC RefreshDC = GetDC(Window);
			int Win32RefreshRate = GetDeviceCaps(RefreshDC, VREFRESH);
			ReleaseDC(Window, RefreshDC);
			if (Win32RefreshRate > 1)
			{
				MonitorRefreshHz = Win32RefreshRate;
			}
			r32 GameUpdateHz = (r32)(MonitorRefreshHz / 2.0f);
			r32 TargetSecondsPerFrame = 1.0f / (r32)GameUpdateHz;

			// TODO: Make this around 60 seconds.
			SoundOutput.SamplesPerSecond = 48000;
			SoundOutput.BytesPerSample = sizeof(i16) * 2;
			SoundOutput.SecondaryBufferSize = SoundOutput.SamplesPerSecond*SoundOutput.BytesPerSample;
			//TODO: Actually compute the variance and see what the lowest resonable value is.
			SoundOutput.SafetyBytes = (int)(((r32)SoundOutput.SamplesPerSecond*(r32)SoundOutput.BytesPerSample / GameUpdateHz) / 3.0f);
			Win32InitDSound(Window, SoundOutput.SamplesPerSecond, SoundOutput.SecondaryBufferSize);
			Win32ClearBuffer(&SoundOutput);
			GlobalSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);

			GlobalRunning = true;

			// TODO: Pool with bitmap VirtualAlloc
			// TODO: Remove MaxPossibleOverrun?
			u32 MaxPossibleOverrun = 2 * 8 * sizeof(u16);
			i16 *Samples = (i16 *)VirtualAlloc(0, SoundOutput.SecondaryBufferSize + MaxPossibleOverrun,
				MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

#ifdef OM_DEBUG
			LPVOID BaseAddress = (LPVOID) om_terabytes(2);
#else
			void *BaseAddress = 0;
#endif

			game_memory GameMemory = {};
			GameMemory.PermanentStorageSize = om_megabytes(256);
			GameMemory.TransientStorageSize = om_megabytes(500);
			GameMemory.HighPriorityQueue = &HighPriorityQueue;
			GameMemory.LowPriorityQueue = &LowPriorityQueue;

			GameMemory.PlatformAPI.AddThreadEntry = Win32AddThreadQueueEntry;
			GameMemory.PlatformAPI.CompleteAllThreadWork = Win32CompleteAllThreadWork;

			GameMemory.PlatformAPI.GetAllFilesOfTypeBegin = Win32GetAllFilesOfTypeBegin;
			GameMemory.PlatformAPI.GetAllFilesOfTypeEnd = Win32GetAllFilesOfTypeEnd;
			GameMemory.PlatformAPI.OpenNextFile = Win32OpenNextFile;
			GameMemory.PlatformAPI.ReadDataFromFile = Win32ReadDataFromFile;
			GameMemory.PlatformAPI.FileError = Win32FileError;

			GameMemory.PlatformAPI.DEBUGFreeFileMemory = DEBUGPlatformFreeFileMemory;
			GameMemory.PlatformAPI.DEBUGReadEntireFile = DEBUGPlatformReadEntireFile;
			GameMemory.PlatformAPI.DEBUGWriteEntireFile = DEBUGPlatformWriteEntireFile;
			//GameMemory.DEBUGLoadBitmap = DEBUGLoadBitmap;

			// TODO: Handle various memory footprints (USING SYSTEM METRICS)
			// TODO: Use MEM_LARGE_PAGES and call adjust token privileges when now on Windows XP?
			
			// TODO: TransientStorage needs to be broken up
			// into game transient and cache transient, and only the
			// former need be saved for state playback.
			Win32State.TotalSize = GameMemory.PermanentStorageSize + GameMemory.TransientStorageSize;
			Win32State.GameMemoryBlock = VirtualAlloc(BaseAddress, (size_t)Win32State.TotalSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
			GameMemory.PermanentStorage = Win32State.GameMemoryBlock;
			GameMemory.TransientStorage = ((u8 *)GameMemory.PermanentStorage + GameMemory.PermanentStorageSize);

			for (int ReplayIndex = 1; ReplayIndex < OM_ARRAYCOUNT(Win32State.ReplayBuffers); ++ReplayIndex)
			{
				win32_replay_buffer *ReplayBuffer = &Win32State.ReplayBuffers[ReplayIndex];

				// TODO: Recording system still seems to take too long
				// on record start - find out what Windows is doing and if
				// we can speed up / defer some of that processing.

				Win32GetInputFileLocation(&Win32State, false, ReplayIndex,
					sizeof(ReplayBuffer->FileName), ReplayBuffer->FileName);

				ReplayBuffer->FileHandle = CreateFileA(ReplayBuffer->FileName, GENERIC_WRITE | GENERIC_READ, 0, 0, CREATE_ALWAYS, 0, 0);

				LARGE_INTEGER MaxSize;
				MaxSize.QuadPart = Win32State.TotalSize;
				ReplayBuffer->MemoryMap = CreateFileMapping(
					ReplayBuffer->FileHandle, 0, PAGE_READWRITE,
					MaxSize.HighPart, MaxSize.LowPart, 0);

				ReplayBuffer->MemoryBlock = MapViewOfFile(
					ReplayBuffer->MemoryMap, FILE_MAP_ALL_ACCESS, 0, 0, Win32State.TotalSize);
				if (ReplayBuffer->MemoryBlock)
				{
				}
				else
				{
					// TODO(casey): Diagnostic
				}
			}

			if (Samples && GameMemory.PermanentStorage && GameMemory.TransientStorage)
			{
				game_input Input[2] = {};
				game_input *NewInput = &Input[0];
				game_input *OldInput = &Input[1];

				LARGE_INTEGER LastCounter = Win32GetWallClock();
				LARGE_INTEGER FlipWallClock = Win32GetWallClock();

				int DebugTimeMarkerIndex = 0;
				win32_debug_time_marker DebugTimeMarkers[30] = { 0 };

				DWORD AudioLatencyBytes = 0;
				r32 AudioLatencySeconds = 0;
				b32 SoundIsValid = false;

				win32_game_code Game = Win32LoadGameCode(SourceGameCodeDLLFullPath, TempGameCodeDLLFullPath, GameCodeLockedFullPath);
				u64 LastCycleCount = __rdtsc();
				while (GlobalRunning)
				{
					NewInput->dtForFrame = TargetSecondsPerFrame;

					//NewInput->ExecutableReloaded = false;
					FILETIME NewDLLWriteTime = Win32GetLastWriteTime(SourceGameCodeDLLFullPath);
					if (CompareFileTime(&NewDLLWriteTime, &Game.DLLLastWriteTime) != 0)
					{
						Win32CompleteAllThreadWork(&HighPriorityQueue);
						Win32CompleteAllThreadWork(&LowPriorityQueue);

						Win32UnloadGameCode(&Game);
						Game = Win32LoadGameCode(SourceGameCodeDLLFullPath, TempGameCodeDLLFullPath, GameCodeLockedFullPath);
						//NewInput->ExecutableReloaded = true;
					}

					//TODO: Zeroing macro
					//TODO: We can't zero everything cause the up/down state will be wrong.
					game_controller_input *OldKeyboardController = GetController(OldInput, 0);
					game_controller_input *NewKeyboardController = GetController(NewInput, 0);
					*NewKeyboardController = {};
					NewKeyboardController->IsConnected = true;
					for (int ButtonIndex = 0; ButtonIndex < OM_ARRAYCOUNT(NewKeyboardController->Buttons); ++ButtonIndex)
					{
						NewKeyboardController->Buttons[ButtonIndex].EndedDown = OldKeyboardController->Buttons[ButtonIndex].EndedDown;
					}

					Win32ProcessPendingMessages(&Win32State, NewKeyboardController);

					if (!GlobalPause)
					{
						POINT MouseP;
						GetCursorPos(&MouseP);
						ScreenToClient(Window, &MouseP);
						//NewInput->MouseX = MouseP.x;
						//NewInput->MouseY = MouseP.y;
						//NewInput->MouseZ = 0; // TODO(casey): Support mousewheel?
						/*Win32ProcessKeyboardMessage(&NewInput->MouseButtons[0],
							GetKeyState(VK_LBUTTON) & (1 << 15));
						Win32ProcessKeyboardMessage(&NewInput->MouseButtons[1],
							GetKeyState(VK_MBUTTON) & (1 << 15));
						Win32ProcessKeyboardMessage(&NewInput->MouseButtons[2],
							GetKeyState(VK_RBUTTON) & (1 << 15));
						Win32ProcessKeyboardMessage(&NewInput->MouseButtons[3],
							GetKeyState(VK_XBUTTON1) & (1 << 15));
						Win32ProcessKeyboardMessage(&NewInput->MouseButtons[4],
							GetKeyState(VK_XBUTTON2) & (1 << 15));*/

						// TODO: Need to not poll disconnected controllers to avoid xinput frame rate hit on older libraries.
						// TODO: Should we poll this more frequently?
						DWORD MaxControllerCount = XUSER_MAX_COUNT;
						if (MaxControllerCount > (OM_ARRAYCOUNT(NewInput->Controllers) - 1))
						{
							MaxControllerCount = (OM_ARRAYCOUNT(NewInput->Controllers) - 1);
						}

						for (DWORD ControllerIndex = 0; ControllerIndex < MaxControllerCount; ++ControllerIndex)
						{
							DWORD OurControllerIndex = ControllerIndex + 1;
							game_controller_input *OldController = GetController(OldInput, OurControllerIndex);
							game_controller_input *NewController = GetController(NewInput, OurControllerIndex);

							XINPUT_STATE ControllerState;
							if (XInputGetState(ControllerIndex, &ControllerState) == ERROR_SUCCESS)
							{
								NewController->IsConnected = true;
								NewController->IsAnalog = OldController->IsAnalog;

								// NOTE: This controller is plugged in.
								// TODO: See if ControllerState.dwPacketNumber increments too rapidly
								XINPUT_GAMEPAD *Pad = &ControllerState.Gamepad;

								// TODO: This is a square deadzone, check XInput to
								// verify that the deadzone is "round" and show how to do
								// round deadzone processing.
								NewController->StickAverageX = Win32ProcessXInputStickValue(
									Pad->sThumbLX, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
								NewController->StickAverageY = Win32ProcessXInputStickValue(
									Pad->sThumbLY, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
								if ((NewController->StickAverageX != 0.0f) ||
									(NewController->StickAverageY != 0.0f))
								{
									NewController->IsAnalog = true;
								}

								if (Pad->wButtons & XINPUT_GAMEPAD_DPAD_UP)
								{
									NewController->StickAverageY = 1.0f;
									NewController->IsAnalog = false;
								}

								if (Pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN)
								{
									NewController->StickAverageY = -1.0f;
									NewController->IsAnalog = false;
								}

								if (Pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT)
								{
									NewController->StickAverageX = -1.0f;
									NewController->IsAnalog = false;
								}

								if (Pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT)
								{
									NewController->StickAverageX = 1.0f;
									NewController->IsAnalog = false;
								}

								r32 Threshold = 0.5f;
								Win32ProcessXInputDigitalButton(
									(NewController->StickAverageX < -Threshold) ? 1 : 0,
									&OldController->MoveLeft, 1,
									&NewController->MoveLeft);
								Win32ProcessXInputDigitalButton(
									(NewController->StickAverageX > Threshold) ? 1 : 0,
									&OldController->MoveRight, 1,
									&NewController->MoveRight);
								Win32ProcessXInputDigitalButton(
									(NewController->StickAverageY < -Threshold) ? 1 : 0,
									&OldController->MoveDown, 1,
									&NewController->MoveDown);
								Win32ProcessXInputDigitalButton(
									(NewController->StickAverageY > Threshold) ? 1 : 0,
									&OldController->MoveUp, 1,
									&NewController->MoveUp);

								Win32ProcessXInputDigitalButton(Pad->wButtons,
									&OldController->ActionDown, XINPUT_GAMEPAD_A,
									&NewController->ActionDown);
								Win32ProcessXInputDigitalButton(Pad->wButtons,
									&OldController->ActionRight, XINPUT_GAMEPAD_B,
									&NewController->ActionRight);
								Win32ProcessXInputDigitalButton(Pad->wButtons,
									&OldController->ActionLeft, XINPUT_GAMEPAD_X,
									&NewController->ActionLeft);
								Win32ProcessXInputDigitalButton(Pad->wButtons,
									&OldController->ActionUp, XINPUT_GAMEPAD_Y,
									&NewController->ActionUp);
								Win32ProcessXInputDigitalButton(Pad->wButtons,
									&OldController->LeftShoulder, XINPUT_GAMEPAD_LEFT_SHOULDER,
									&NewController->LeftShoulder);
								Win32ProcessXInputDigitalButton(Pad->wButtons,
									&OldController->RightShoulder, XINPUT_GAMEPAD_RIGHT_SHOULDER,
									&NewController->RightShoulder);

								Win32ProcessXInputDigitalButton(Pad->wButtons,
									&OldController->Start, XINPUT_GAMEPAD_START,
									&NewController->Start);
								Win32ProcessXInputDigitalButton(Pad->wButtons,
									&OldController->Back, XINPUT_GAMEPAD_BACK,
									&NewController->Back);
							}
							else
							{
								// NOTE: This controller is not available
								NewController->IsConnected = false;
							}
						}

						game_offscreen_buffer Buffer = {};
						Buffer.Memory = GlobalBackbuffer.Memory;
						Buffer.Width = GlobalBackbuffer.Width;
						Buffer.Height = GlobalBackbuffer.Height;
						Buffer.Pitch = GlobalBackbuffer.Pitch;
						Buffer.BytesPerPixel = GlobalBackbuffer.BytesPerPixel;

						if (Win32State.InputRecordingIndex)
						{
							Win32RecordInput(&Win32State, NewInput);
						}

						if (Win32State.InputPlayingIndex)
						{
							Win32PlayBackInput(&Win32State, NewInput);
						}

						if (Game.UpdateAndRender)
						{
							Game.UpdateAndRender(&GameMemory, NewInput, &Buffer);
							//HandleDebugCycleCounters(&GameMemory);
							HandleDebugTimeCounters(&GameMemory);
						}

						LARGE_INTEGER AudioWallClock = Win32GetWallClock();
						r32 FromBeginToAudioSeconds = Win32GetSecondsElapsed(FlipWallClock, AudioWallClock);

						DWORD PlayCursor;
						DWORD WriteCursor;
						if (GlobalSecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor) == DS_OK)
						{
							/* NOTE:
							   Here is how sound output computation works.
							   We define a safety value that is the number
							   of samples we think our game update loop
							   may vary by (let's say up to 2ms)

							   When we wake up to write audio, we will look
							   and see what the play cursor position is and we
							   will forecast ahead where we think the play
							   cursor will be on the next frame boundary.
							   We will then look to see if the write cursor is
							   before that by at least our safety value.  If
							   it is, the target fill position is that frame
							   boundary plus one frame.  This gives us perfect
							   audio sync in the case of a card that has low
							   enough latency.
							   If the write cursor is _after_ that safety
							   margin, then we assume we can never sync the
							   audio perfectly, so we will write one frame's
							   worth of audio plus the safety margin's worth
							   of guard samples.
							*/

							if (!SoundIsValid)
							{
								SoundOutput.RunningSampleIndex = WriteCursor / SoundOutput.BytesPerSample;
								SoundIsValid = true;
							}

							DWORD ByteToLock = ((SoundOutput.RunningSampleIndex*SoundOutput.BytesPerSample) % SoundOutput.SecondaryBufferSize);

							DWORD ExpectedSoundBytesPerFrame = (int)((r32)(SoundOutput.SamplesPerSecond*SoundOutput.BytesPerSample) / GameUpdateHz);
							r32 SecondsLeftUntilFlip = (TargetSecondsPerFrame - FromBeginToAudioSeconds);
							DWORD ExpectedBytesUntilFlip = (DWORD)((SecondsLeftUntilFlip / TargetSecondsPerFrame) * (r32)ExpectedSoundBytesPerFrame);

							DWORD ExpectedFrameBoundaryByte = PlayCursor + ExpectedBytesUntilFlip;

							DWORD SafeWriteCursor = WriteCursor;
							if (SafeWriteCursor < PlayCursor)
							{
								SafeWriteCursor += SoundOutput.SecondaryBufferSize;
							}

							OM_ASSERT(SafeWriteCursor >= PlayCursor);
							SafeWriteCursor += SoundOutput.SafetyBytes;

							b32 AudioCardIsLowLatency = (SafeWriteCursor < ExpectedFrameBoundaryByte);

							DWORD TargetCursor = 0;
							if (AudioCardIsLowLatency)
							{
								TargetCursor = (ExpectedFrameBoundaryByte + ExpectedSoundBytesPerFrame);
							}
							else
							{
								TargetCursor = (WriteCursor + ExpectedSoundBytesPerFrame + SoundOutput.SafetyBytes);
							}

							TargetCursor = (TargetCursor % SoundOutput.SecondaryBufferSize);
							
							DWORD BytesToWrite = 0;
							if (ByteToLock > TargetCursor)
							{
								BytesToWrite = (SoundOutput.SecondaryBufferSize - ByteToLock);
								BytesToWrite += TargetCursor;
							}
							else
							{
								BytesToWrite = TargetCursor - ByteToLock;
							}

							game_sound_output_buffer SoundBuffer = {};
							SoundBuffer.SamplesPerSecond = SoundOutput.SamplesPerSecond;
							SoundBuffer.SampleCount = Align8(BytesToWrite / SoundOutput.BytesPerSample);
							BytesToWrite = SoundBuffer.SampleCount*SoundOutput.BytesPerSample;
							SoundBuffer.Samples = Samples;
							if (Game.GetSoundSamples)
							{
								Game.GetSoundSamples(&GameMemory, &SoundBuffer);
							}

#if 0
							win32_debug_time_marker *Marker = &DebugTimeMarkers[DebugTimeMarkerIndex];
							Marker->OutputPlayCursor = PlayCursor;
							Marker->OutputWriteCursor = WriteCursor;
							Marker->OutputLocation = ByteToLock;
							Marker->OutputByteCount = BytesToWrite;
							Marker->ExpectedFlipPlayCursor = ExpectedFrameBoundaryByte;

							DWORD UnwrappedWriteCursor = WriteCursor;
							if (UnwrappedWriteCursor < PlayCursor)
							{
								UnwrappedWriteCursor += SoundOutput.SecondaryBufferSize;
							}
							AudioLatencyBytes = UnwrappedWriteCursor - PlayCursor;
							AudioLatencySeconds = (((r32)AudioLatencyBytes / (r32)SoundOutput.BytesPerSample) / (r32)SoundOutput.SamplesPerSecond);

#if 0
							char TextBuffer[256];
							_snprintf_s(TextBuffer, sizeof(TextBuffer),
								"BTL:%u TC:%u BTW:%u - PC:%u WC:%u DELTA:%u (%fs)\n",
								ByteToLock, TargetCursor, BytesToWrite,
								PlayCursor, WriteCursor, AudioLatencyBytes, AudioLatencySeconds);
							OutputDebugStringA(TextBuffer);
#endif
#endif
							Win32FillSoundBuffer(&SoundOutput, ByteToLock, BytesToWrite, &SoundBuffer);
						}
						else
						{
							SoundIsValid = false;
						}

						LARGE_INTEGER WorkCounter = Win32GetWallClock();
						r32 WorkSecondsElapsed = Win32GetSecondsElapsed(LastCounter, WorkCounter);

						// TODO: Not tested and this is probably buggy.
						//r32 SecondsElapsedForFrame = WorkSecondsElapsed;
						//if (SecondsElapsedForFrame < TargetSecondsPerFrame)
						//{
						//	if (SleepIsGranular)
						//	{
						//		DWORD SleepMS = (DWORD)(1000.0f * (TargetSecondsPerFrame - SecondsElapsedForFrame));

						//		if (SleepMS > 0)
						//		{
						//			Sleep(SleepMS);
						//		}
						//	}

						//	r32 TestSecondsElapsedForFrame = Win32GetSecondsElapsed(LastCounter, Win32GetWallClock());
						//	if (TestSecondsElapsedForFrame < TargetSecondsPerFrame)
						//	{
						//		// TODO: Log missed sleep.
						//	}

						//	while (SecondsElapsedForFrame < TargetSecondsPerFrame)
						//	{
						//		SecondsElapsedForFrame = Win32GetSecondsElapsed(LastCounter, Win32GetWallClock());
						//	}
						//}
						//else
						//{
						//	// TODO: MISSED FRAME RATE!
						//	// TODO: Logging
						//}

						LARGE_INTEGER EndCounter = Win32GetWallClock();
						r32 MSPerFrame = 1000.0f * Win32GetSecondsElapsed(LastCounter, EndCounter);
						LastCounter = EndCounter;

						win32_window_dimension Dimension = Win32GetWindowDimension(Window);
						HDC DeviceContext = GetDC(Window);
						Win32DisplayBufferInWindow(&GlobalBackbuffer, DeviceContext, Dimension.Width, Dimension.Height);
						ReleaseDC(Window, DeviceContext);

						FlipWallClock = Win32GetWallClock();

#if 0
						// NOTE: THIS IS DEBUG CODE
						{
							DWORD PlayCursor;
							DWORD WriteCursor;
							if (GlobalSecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor) == DS_OK)
							{
								OM_ASSERT(DebugTimeMarkerIndex < OM_ARRAYCOUNT(DebugTimeMarkers));
								win32_debug_time_marker *Marker = &DebugTimeMarkers[DebugTimeMarkerIndex];
								Marker->FlipPlayCursor = PlayCursor;
								Marker->FlipWriteCursor = WriteCursor;
							}
						}
#endif

						game_input *Temp = NewInput;
						NewInput = OldInput;
						OldInput = Temp;
						// TODO: Should theese be cleared here?

#if 1
						u64 EndCycleCount = __rdtsc();
						u64 CyclesElapsed = EndCycleCount - LastCycleCount;
						LastCycleCount = EndCycleCount;

						r64 MCPF = ((r64)CyclesElapsed / (1000.0f * 1000.0f));
						r64 FPS = 1000.0f / MSPerFrame;

						char FPSBuffer[256];
						_snprintf_s(FPSBuffer, sizeof(FPSBuffer),
							"%.02fms/f,  %.02ff/s,  %.02fmc/f\n", MSPerFrame, FPS, MCPF);
						OutputDebugStringA(FPSBuffer);
#endif

#if 0
						++DebugTimeMarkerIndex;
						if (DebugTimeMarkerIndex == ArrayCount(DebugTimeMarkers))
						{
							DebugTimeMarkerIndex = 0;
						}
#endif
					}
				}
			}
			else
			{
				// TODO: Logging
			}
		}
		else
		{
			// TODO: Logging
		}
	}
	else
	{
		// TODO: Logging
	}

	return (0);
}