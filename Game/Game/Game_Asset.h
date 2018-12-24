#ifndef GAME_ASSET_H
#define GAME_ASSET_H
#pragma once

struct loaded_sound
{
	u32 SampleCount;
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

//TODO: The asset tags are currently temporary and will need to be changed into something sane once we know what kind of assets
// the game will support.
enum asset_tag_id
{
	Asset_Tag_Roundness,
	Asset_Tag_Flatness,

	Asset_Tag_Count
};

enum asset_type_id
{
	Asset_Type_None,

	//Note: Bitmaps
	Asset_Type_Grass,
	Asset_Type_Water,
	Asset_Type_SlopeLeft,
	Asset_Type_SlopeRight,
	Asset_Type_Player,

	//Note: Sounds
	Asset_Type_Music,

	Asset_Type_Count
};

struct asset_tag
{
	u32 ID; //Note: Tag ID.
	r32 Value;
};

struct asset_type
{
	u32 FirstAssetIndex;		//Note: Range of all the assets to consider for this asset type.
	u32 OnePastLastAssetIndex;
};

struct asset
{
	u32 FirstTagIndex;			//Note: Range of all the tags to consider for this asset.
	u32 OnePastLastTagIndex;
	u32 SlotID;
};

struct asset_vector
{
	r32 E[Asset_Tag_Count];
};

struct asset_bitmap_info
{
	char *FileName;
	//Note: Additional bitmap information later stored in the asset files could be added here.
};

struct asset_sound_info
{
	char *FileName;
	//Note: Additional sound information later stored in the asset files could be added here.
};

struct game_assets
{
	// TODO: This back-pointer is dumb.
	struct transient_state *TransientState;
	memory_arena Arena;
	
	r32 TagRange[Asset_Tag_Count];

	u32 BitmapCount;
	asset_bitmap_info *BitmapInfos;
	asset_slot *Bitmaps;

	u32 SoundCount;
	asset_sound_info *SoundInfos;
	asset_slot *Sounds;

	u32 TagCount;
	asset_tag *Tags;

	u32 AssetCount;
	asset *Assets;

	asset_type AssetTypes[Asset_Type_Count];

	//TODO: Remove these example array'd assets. These are only for example!!!
	loaded_bitmap Stone[4];
	test_structured_asset Characters[4];

	//TODO: Temp, should be removed once we load packed asset files.
	u32 DEBUGUsedBitmapCount;
	u32 DEBUGUsedSoundCount;
	u32 DEBUGUsedAssetCount;
	u32 DEBUGUsedTagCount;
	asset_type *DEBUGAssetType;
	asset *DEBUGAsset;
};

struct bitmap_id
{
	u32 Value;
};

struct sound_id
{
	u32 Value;
};

inline loaded_bitmap *
GetBitmap(game_assets *Assets, bitmap_id ID)
{
	loaded_bitmap *Result = Assets->Bitmaps[ID.Value].Bitmap;

	return (Result);
}

inline loaded_sound *
GetSound(game_assets *Assets, sound_id ID)
{
	loaded_sound *Result = Assets->Sounds[ID.Value].Sound;

	return (Result);
}

om_internal void LoadBitmap(game_assets *Assets, bitmap_id ID);
om_internal void LoadSound(game_assets *Assets, sound_id ID);

#endif GAME_ASSET_H
