#ifndef GAME_ASSET_TYPE_ID_H
#define GAME_ASSET_TYPE_ID_H
#pragma once

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

#endif //GAME_ASSET_TYPE_ID_H
