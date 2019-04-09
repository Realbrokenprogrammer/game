#ifndef GAME_ASSET_H
#define GAME_ASSET_H
#pragma once

struct bitmap_id
{
	u32 Value;
};

struct sound_id
{
	u32 Value;
};

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

struct asset_tag
{
	u32 ID; //Note: Tag ID.
	r32 Value;
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
	u32 FirstTagIndex;			//Note: Range of all the tags to consider for this asset.
	u32 OnePastLastTagIndex;

	union
	{
		asset_bitmap_info Bitmap;
		asset_sound_info Sound;
	};
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

struct game_assets
{
	// TODO: This back-pointer is dumb.
	struct transient_state *TransientState;
	memory_arena Arena;
	
	r32 TagRange[Asset_Tag_Count];

	u32 TagCount;
	asset_tag *Tags;

	u32 AssetCount;
	asset *Assets;
	asset_slot *Slots;

	asset_type AssetTypes[Asset_Type_Count];

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
	loaded_bitmap *Result = Assets->Slots[ID.Value].Bitmap;

	return (Result);
}

inline loaded_sound *
GetSound(game_assets *Assets, sound_id ID)
{
	OM_ASSERT(ID.Value <= Assets->AssetCount);
	loaded_sound *Result = Assets->Slots[ID.Value].Sound;

	return (Result);
}

inline asset_sound_info *
GetSoundInfo(game_assets *Assets, sound_id ID)
{
	OM_ASSERT(ID.Value <= Assets->AssetCount);
	asset_sound_info *Result = &Assets->Assets[ID.Value].Sound;

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
