#ifndef GAME_ASSET_H
#define GAME_ASSET_H
#pragma once

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
	loaded_bitmap *Bitmap;
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

	Asset_Type_Grass,
	Asset_Type_Water,
	Asset_Type_SlopeLeft,
	Asset_Type_SlopeRight,
	Asset_Type_Player,

	Asset_Type_Count
};

struct asset_tag
{
	u32 ID; //Note: Tag ID.
	r32 Value;
};

struct asset_type
{
	u32 FirstAssetIndex;
	u32 OnePastLastAssetIndex;
};

struct asset
{
	u32 FirstTagIndex;
	u32 OnePastLastTagIndex;
	u32 SlotID;
};

struct asset_bitmap_info
{
	i32 Width;
	i32 Height;
	//Note: Additional bitmap information later stored in the asset files could be added here.
};

struct game_assets
{
	// TODO: This back-pointer is dumb.
	struct transient_state *TransientState;
	memory_arena Arena;
	
	u32 BitmapCount;
	asset_slot *Bitmaps;

	u32 SoundCount;
	asset_slot *Sounds;

	u32 TagCount;
	asset_tag *Tags;

	u32 AssetCount;
	asset *Assets;

	asset_type AssetTypes[Asset_Type_Count];

	//TODO: Remove these example array'd assets. These are only for example!!!
	loaded_bitmap Stone[4];
	test_structured_asset Characters[4];
};

struct bitmap_id
{
	u32 Value;
};

struct audio_id
{
	u32 Value;
};

inline loaded_bitmap *
GetBitmap(game_assets *Assets, bitmap_id ID)
{
	loaded_bitmap *Result = Assets->Bitmaps[ID.Value].Bitmap;

	return (Result);
}

om_internal void LoadBitmap(game_assets *Assets, bitmap_id ID);
om_internal void LoadSound(game_assets *Assets, audio_id ID);

#endif GAME_ASSET_H
