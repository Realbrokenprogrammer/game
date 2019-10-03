#ifndef ASSET_BUILDER_H
#define ASSET_BUILDER_H
#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include "../../Game/Game/Types.h"
#include "../../Game/Game/Game_Asset_Type_Id.h"
#include "../../Game/Game/Game_File_Formats.h"
#include "../../Game/Game/Game_Intristics.h"
#include "../../Game/Game/Game_Math.h"

//#define USE_WINDOWS_API 0
#define LARGE_NUMBER 4096

#ifdef USE_WINDOWS_API
#include <windows.h>
#else
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"
#endif

struct loaded_bitmap
{
	i32 Width;
	i32 Height;
	void *Memory;
	i32 Pitch;

    void *Free;
};

struct loaded_sound
{
	u32 SampleCount; // NOTE: This is the sample count divided by 8
	u32 ChannelCount;
	i16 *Samples[2];

    void *Free;
};

struct asset_bitmap_info
{
	char *FileName;
	//Note: Additional bitmap information later stored in the asset files could be added here.
};

struct asset_sound_info
{
	char *FileName;
	u32 FirstSampleIndex;
	u32 SampleCount;
	sound_id NextIDToPlay;
	//Note: Additional sound information later stored in the asset files could be added here.
};

enum asset_type
{
    AssetType_Bitmap,
    AssetType_Sound,
	AssetType_Font,
};

struct asset_source
{
    asset_type Type;
    char *FileName;
    union
    {
        u32 FirstSampleIndex;
        u32 Codepoint;
    };
};

struct game_assets
{
    u32 TagCount;
    ga_tag Tags[LARGE_NUMBER];

    u32 AssetTypeCount;
    ga_asset_type AssetTypes[Asset_Type_Count];

    u32 AssetCount;
    asset_source AssetSources[LARGE_NUMBER];
    ga_asset Assets[LARGE_NUMBER];

    ga_asset_type *DEBUGAssetType;
    u32 AssetIndex;
};

inline b32
IsValid(bitmap_id ID)
{
	b32 Result = (ID.Value != 0);
	
	return (Result);
}

inline b32
IsValid(sound_id ID)
{
	b32 Result = (ID.Value != 0);

	return (Result);
}

#endif // ASSET_BUILDER