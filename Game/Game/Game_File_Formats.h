#ifndef GAME_FILE_FORMATS_H
#define GAME_FILE_FORMATS_H
#pragma once

//TODO: The asset tags are currently temporary and will need to be changed into something sane once we know what kind of assets
// the game will support.
enum asset_tag_id
{
	Asset_Tag_Roundness,
	Asset_Tag_Flatness,

	Asset_Tag_UnicodeCodepoint,

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
	
	//Note: Fonts
	Asset_Type_Font,

	//Note: Sounds
	Asset_Type_Music,

	Asset_Type_Count
};

struct bitmap_id
{
	u32 Value;
};

struct sound_id
{
	u32 Value;
};

#pragma pack(push, 1)
struct ga_header
{
#define GA_MAGIC_VALUE 69
	u32 MagicValue;

#define GA_VERSION 0
	u32 Version;

	u32 TagCount;
	u32 AssetTypeCount;
	u32 AssetCount;

	u64 TagsOffset;				// ga_tag[TagCount]
	u64 AssetTypesOffset;		// ga_asset_type[AssetTypeCount]
	u64 AssetsOffset;			// ga_asset[AssetCount] 
};

struct ga_tag
{
	u32 ID;
	r32 Value;
};

struct ga_asset_type
{
	u32 TypeID;
	u32 FirstAssetIndex;
	u32 OnePastLastAssetIndex;
};

enum ga_sound_chain
{
	GASoundChain_None,
	GASoundChain_Loop,
	GASoundChain_Advance,
};

struct ga_bitmap
{
	u32 Dimension[2]; // NOTE: Bitmap width and height.
};

struct ga_sound
{
	u32 SampleCount;
	u32 ChannelCount;
	u32 Chain;
};

struct ga_asset
{
	u64 DataOffset;
	u32 FirstTagIndex;
	u32 OnePastLastTagIndex;

	union
	{
		ga_bitmap Bitmap;
		ga_sound Sound;
	};
};

#pragma pack(pop)

#endif //GAME_FILE_FORMATS_H