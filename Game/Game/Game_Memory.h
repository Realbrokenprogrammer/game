#ifndef GAME_MEMORY_H
#define GAME_MEMORY_H
#pragma once

struct memory_arena
{
	memory_index Size;
	u8 *Base;
	memory_index Used;

	i32 TempCount;
};

struct temporary_memory
{
	memory_arena *Arena;
	memory_index Used;
};

inline void
InitializeMemoryArena(memory_arena *Arena, memory_index Size, void *Base)
{
	Arena->Size = Size;
	Arena->Base = (u8 *)Base;
	Arena->Used = 0;
	Arena->TempCount = 0;
}

inline memory_index
GetMemoryAlignmentOffset(memory_arena *Arena, memory_index Alignment)
{
	memory_index AlignmentOffset = 0;

	memory_index ResultPointer = (memory_index)Arena->Base + Arena->Used;
	memory_index AlignmentMask = Alignment - 1;
	if (ResultPointer & AlignmentMask)
	{
		AlignmentOffset = Alignment - (ResultPointer & AlignmentMask);
	}

	return (AlignmentOffset);
}

inline memory_index
GetMemoryArenaSizeRemaining(memory_arena *Arena, memory_index Alignment = 4)
{
	memory_index Result = Arena->Size - (Arena->Used + GetMemoryAlignmentOffset(Arena, Alignment));

	return (Result);
}

#define PushStruct(Arena, Type, ...) (Type *)PushSize_(Arena, sizeof(Type), ## __VA_ARGS__)
#define PushArray(Arena, Count, Type, ...) (Type *) PushSize_(Arena, (Count)*sizeof(Type), ## __VA_ARGS__)
#define PushSize(Arena, Size, ...) PushSize_(Arena, Size, ## __VA_ARGS__)

inline void *
PushSize_(memory_arena *Arena, memory_index SizeInit, memory_index Alignment = 4)
{
	memory_index Size = SizeInit;

	memory_index AlignmentOffset = GetMemoryAlignmentOffset(Arena, Alignment);
	Size += AlignmentOffset;

	OM_ASSERT((Arena->Used + Size) <= Arena->Size);

	void *Result = Arena->Base + Arena->Used + AlignmentOffset;
	Arena->Used += Size;

	OM_ASSERT(Size >= SizeInit);

	return (Result);
}

//NOTE: Generally not for production use.
inline char *
PushString(memory_arena *Arena, char *Source)
{
	u32 Size = 1;
	for (char *At = Source; *At; ++At)
	{
		++Size;
	}

	char *Destination = (char *)PushSize_(Arena, Size);
	for (u32 CharIndex = 0; CharIndex < Size; ++CharIndex)
	{
		Destination[CharIndex] = Source[CharIndex];
	}

	return (Destination);
}

inline temporary_memory
CreateTemporaryMemory(memory_arena *Arena)
{
	temporary_memory Result;

	Result.Arena = Arena;
	Result.Used = Arena->Used;

	++Arena->TempCount;

	return (Result);
}

inline void
DestroyTemporaryMemory(temporary_memory TemporaryMemory)
{
	memory_arena *Arena = TemporaryMemory.Arena;
	OM_ASSERT(Arena->Used >= TemporaryMemory.Used);

	Arena->Used = TemporaryMemory.Used;
	OM_ASSERT(Arena->TempCount > 0);

	--Arena->TempCount;
}

inline void
CheckMemoryArena(memory_arena *Arena)
{
	OM_ASSERT(Arena->TempCount == 0);
}

inline void
CreateSubArena(memory_arena *Result, memory_arena *Arena, memory_index Size, memory_index Alignment = 16)
{
	Result->Size = Size;
	Result->Base = (u8 *)PushSize_(Arena, Size, Alignment);
	Result->Used = 0;
	Result->TempCount = 0;
}

#define ZeroStruct(Instance) ZeroSize(sizeof(Instance), &(Instance))

inline void
ZeroSize(memory_index Size, void *Pointer)
{
	//TODO: Performance check
	u8 *Byte = (u8 *)Pointer;
	while (Size--)
	{
		*Byte++ = 0;
	}
}

inline void
Copy(memory_index Size, void *SourceInit, void *DestinationInit)
{
	u8 *Source = (u8 *)SourceInit;
	u8 *Destination = (u8 *)DestinationInit;

	while (Size--) { *Destination++ = *Source++; }
}

#endif // GAME_MEMORY_H