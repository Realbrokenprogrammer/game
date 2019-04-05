#ifndef GAME_FILE_FORMATS_H
#define GAME_FILE_FORMATS_H
#pragma once

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

struct ga_bitmap
{
	u32 Dimension[2]; // NOTE: Bitmap width and height.
};

struct ga_sound
{
	u32 FirstSampleIndex;
	u32 SampleCount;
	u32 NextIDToPlay;
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