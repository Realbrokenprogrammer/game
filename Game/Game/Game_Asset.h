#ifndef GAME_ASSET_H
#define GAME_ASSET_H
#pragma once

struct loaded_sound
{
	u32 SampleCount; // NOTE: This is the sample count divided by 8
	u32 ChannelCount;
	i16 *Samples[2];
};

//TODO: This is a test example to see how a structured asset would look like.
struct test_structured_asset
{
	loaded_bitmap Head;
	loaded_bitmap Torso;
	loaded_bitmap Legs;
};

enum asset_state
{
	AssetState_Unloaded,
	AssetState_Queued,
	AssetState_Loaded,
	AssetState_Locked
};

struct asset_slot
{
	asset_state State;
	union
	{
		loaded_bitmap *Bitmap;
		loaded_sound *Sound;
	};
};

struct asset
{
	ga_asset GA;
	u32 FileIndex;
};

struct asset_vector
{
	r32 E[Asset_Tag_Count];
};

struct asset_type
{
	u32 FirstAssetIndex;		//Note: Range of all the assets to consider for this asset type.
	u32 OnePastLastAssetIndex;
};

struct asset_file
{
	platform_file_handle *Handle;

	ga_header Header;
	ga_asset_type *AssetTypeArray;

	u32 TagBase;
};

struct game_assets
{
	// TODO: This back-pointer is dumb.
	struct transient_state *TransientState;
	memory_arena Arena;
	
	r32 TagRange[Asset_Tag_Count];

	u32 FileCount;
	asset_file *Files;

	u32 TagCount;
	ga_tag *Tags;

	u32 AssetCount;
	asset *Assets;
	asset_slot *Slots;

	asset_type AssetTypes[Asset_Type_Count];

	u8 *GAContents;

#if 0
	//TODO: Remove these example array'd assets. These are only for example!!!
	loaded_bitmap Stone[4];
	test_structured_asset Characters[4];

	//TODO: Temp, should be removed once we load packed asset files.
	u32 DEBUGUsedAssetCount;
	u32 DEBUGUsedTagCount;
	asset_type *DEBUGAssetType;
	asset *DEBUGAsset;
#endif
};

inline loaded_bitmap *
GetBitmap(game_assets *Assets, bitmap_id ID)
{
	OM_ASSERT(ID.Value <= Assets->AssetCount);
	asset_slot *Slot = Assets->Slots + ID.Value;
	
	loaded_bitmap *Result = 0;
	if (Slot->State >= AssetState_Loaded)
	{
		CompletePreviousReadsBeforeFutureReads;
		Result = Slot->Bitmap;
	}

	return (Result);
}

inline loaded_sound *
GetSound(game_assets *Assets, sound_id ID)
{
	OM_ASSERT(ID.Value <= Assets->AssetCount);
	asset_slot *Slot = Assets->Slots + ID.Value;
	
	loaded_sound *Result = 0;
	if (Slot->State >= AssetState_Loaded)
	{
		CompletePreviousReadsBeforeFutureReads;
		Result = Slot->Sound;
	}

	return (Result);
}

inline ga_sound *
GetSoundInfo(game_assets *Assets, sound_id ID)
{
	OM_ASSERT(ID.Value <= Assets->AssetCount);
	ga_sound *Result = &Assets->Assets[ID.Value].GA.Sound;

	return (Result);
}

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

om_internal void LoadBitmap(game_assets *Assets, bitmap_id ID);
om_internal void LoadSound(game_assets *Assets, sound_id ID);
inline void PrefetchSound(game_assets *Assets, sound_id ID) { LoadSound(Assets, ID); }

#endif GAME_ASSET_H
