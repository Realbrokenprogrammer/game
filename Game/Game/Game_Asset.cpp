
// Packing struct to avoid padding.
// Source for bitmap header: https://www.fileformat.info/format/bmp/egff.htm
#pragma pack(push, 1)
struct bitmap_header
{
	u16 FileType;
	u32 FileSize;
	u16 Reserved1;
	u16 Reserved2;
	u32 BitmapOffset;
	u32 Size;
	i32 Width;
	i32 Height;
	u16 Planes;
	u16 BitsPerPixel;
	u32 Compression;
	u32 SizeOfBitmap;
	i32 HorzResolution;
	i32 VertResolution;
	u32 ColorsUsed;
	u32 ColorsImportant;

	u32 RedMask;
	u32 GreenMask;
	u32 BlueMask;
};
#pragma pack(pop)


//Note: This is not complete bitmap loading code hence it should only be used as such.
om_internal loaded_bitmap
DEBUGLoadBitmap(char* FileName)
{
	loaded_bitmap Result = {};

	debug_read_file_result ReadResult = DEBUGPlatformReadEntireFile(FileName);
	if (ReadResult.ContentsSize != 0)
	{
		bitmap_header *Header = (bitmap_header *)ReadResult.Contents;

		u32 *Pixels = (u32 *)((u8 *)ReadResult.Contents + Header->BitmapOffset);

		Result.Pixels = Pixels;
		Result.Width = Header->Width;
		Result.Height = Header->Height;

		OM_ASSERT(Header->Compression == 3);

		u32 RedMask = Header->RedMask;
		u32 GreenMask = Header->GreenMask;
		u32 BlueMask = Header->BlueMask;
		u32 AlphaMask = ~(RedMask | GreenMask | BlueMask);

		// Bitscan instrinsics to find out how much we need to shift the values down.
		bit_scan_result RedShift = FindLeastSignificantSetBit(RedMask);
		bit_scan_result GreenShift = FindLeastSignificantSetBit(GreenMask);
		bit_scan_result BlueShift = FindLeastSignificantSetBit(BlueMask);
		bit_scan_result AlphaShift = FindLeastSignificantSetBit(AlphaMask);

		OM_ASSERT(RedShift.Found);
		OM_ASSERT(GreenShift.Found);
		OM_ASSERT(BlueShift.Found);
		OM_ASSERT(AlphaShift.Found);

		u32 *SourceDestination = Pixels;
		for (i32 Y = 0; Y < Header->Height; ++Y)
		{
			for (i32 X = 0; X < Header->Width; ++X)
			{
				u32 C = *SourceDestination;
				*SourceDestination++ = ((((C >> AlphaShift.Index) & 0xFF) << 24) |
					(((C >> RedShift.Index) & 0xFF) << 16) |
					(((C >> GreenShift.Index) & 0xFF) << 8) |
					(((C >> BlueShift.Index) & 0xFF) << 0));
			}
		}
	}

	Result.Pitch = Result.Width*BITMAP_BYTES_PER_PIXEL;

	//Note: Changes the pixels to point at the last row and makes the pitch negative to resolve bitmaps
	//being stored upside down.
#if 1
	Result.Pixels = (u32 *)((u8 *)Result.Pixels + Result.Pitch*(Result.Height - 1));
	Result.Pitch = -Result.Pitch;
#endif

	return (Result);
}

struct load_asset_work
{
	game_assets *Assets;
	bitmap_id ID;
	task_with_memory *Task;
	loaded_bitmap *Bitmap;

	asset_state FinalState;
};
om_internal
PLATFORM_THREAD_QUEUE_CALLBACK(LoadAssetWork)
{
	load_asset_work *Work = (load_asset_work *)Data;

	asset_bitmap_info *Info = Work->Assets->BitmapInfos + Work->ID.Value;
	*Work->Bitmap = DEBUGLoadBitmap(Info->FileName);

	CompletePreviousWritesBeforeFutureWrites;

	Work->Assets->Bitmaps[Work->ID.Value].Bitmap = Work->Bitmap;
	Work->Assets->Bitmaps[Work->ID.Value].State = Work->FinalState;

	EndTaskWithMemory(Work->Task);
}

om_internal void
LoadBitmap(game_assets *Assets, bitmap_id ID)
{
	if (ID.Value && 
		(AtomicCompareExchangeUInt32((u32 *)&Assets->Bitmaps[ID.Value].State, AssetState_Unloaded, AssetState_Queued) == AssetState_Unloaded))
	{
		task_with_memory *Task = BeginTaskWithMemory(Assets->TransientState);
		if (Task)
		{
			load_asset_work *Work = PushStruct(&Task->Arena, load_asset_work);;
			Work->Assets = Assets;
			Work->ID = ID;
			Work->Task = Task;
			Work->Bitmap = PushStruct(&Assets->Arena, loaded_bitmap);
			Work->FinalState = AssetState_Loaded;

			PlatformAddThreadEntry(Assets->TransientState->LowPriorityQueue, LoadAssetWork, Work);
		}
	}
}

om_internal void
LoadSound(game_assets *Assets, audio_id ID)
{
	//TODO: Implement this.
}

om_internal bitmap_id
GetFirstBitmapID(game_assets *Assets, asset_type_id TypeID)
{
	bitmap_id Result = {};

	asset_type *Type = Assets->AssetTypes + TypeID;
	if (Type->FirstAssetIndex != Type->OnePastLastAssetIndex)
	{
		asset *Asset = Assets->Assets + Type->FirstAssetIndex;
		Result.Value = Asset->SlotID;
	}

	return (Result);
}

om_internal bitmap_id
DEBUGAddBitmapInfo(game_assets *Assets, char *FileName)
{
	OM_ASSERT(Assets->DEBUGUsedBitmapCount < Assets->BitmapCount);

	bitmap_id ID = { Assets->DEBUGUsedBitmapCount++ };

	asset_bitmap_info *Info = Assets->BitmapInfos + ID.Value;
	Info->FileName = FileName;

	return (ID);
}

om_internal void
BeginAssetType(game_assets *Assets, asset_type_id TypeID)
{
	OM_ASSERT(Assets->DEBUGAssetType == 0);

	Assets->DEBUGAssetType = Assets->AssetTypes + TypeID;
	Assets->DEBUGAssetType->FirstAssetIndex = Assets->DEBUGUsedAssetCount;
	Assets->DEBUGAssetType->OnePastLastAssetIndex = Assets->DEBUGAssetType->FirstAssetIndex;
}

om_internal void
AddBitmapAsset(game_assets *Assets, char *FileName)
{
	OM_ASSERT(Assets->DEBUGAssetType);

	asset *Asset = Assets->Assets + Assets->DEBUGAssetType->OnePastLastAssetIndex++;
	Asset->FirstTagIndex = 0;
	Asset->OnePastLastTagIndex = 0;
	Asset->SlotID = DEBUGAddBitmapInfo(Assets, FileName).Value;
}

om_internal void
EndAssetType(game_assets *Assets)
{
	OM_ASSERT(Assets->DEBUGAssetType);
	Assets->DEBUGUsedAssetCount = Assets->DEBUGAssetType->OnePastLastAssetIndex;
	Assets->DEBUGAssetType = 0;
}

om_internal game_assets *
CreateGameAssets(memory_arena *Arena, memory_index Size, transient_state *TransientState)
{
	game_assets *Assets = PushStruct(Arena, game_assets);
	CreateSubArena(&Assets->Arena, Arena, Size);
	Assets->TransientState = TransientState;

	Assets->BitmapCount = 256*Asset_Type_Count; //TODO: Temporary large value for debugging.
	Assets->BitmapInfos = PushArray(Arena, Assets->BitmapCount, asset_bitmap_info);
	Assets->Bitmaps = PushArray(Arena, Assets->BitmapCount, asset_slot);

	//TODO: There is currently no sounds so this is just temporary.
	Assets->SoundCount = 1;
	Assets->Sounds = PushArray(Arena, Assets->SoundCount, asset_slot);

	Assets->TagCount = 0;
	Assets->Tags = 0;

	Assets->AssetCount = Assets->SoundCount + Assets->BitmapCount;
	Assets->Assets = PushArray(Arena, Assets->AssetCount, asset);

	Assets->DEBUGUsedBitmapCount = 1;
	Assets->DEBUGUsedAssetCount = 1;

	BeginAssetType(Assets, Asset_Type_Grass);
	AddBitmapAsset(Assets, "C:\\Users\\Oskar\\Documents\\GitHub\\game\\Data\\groundTile.bmp");
	EndAssetType(Assets);

	BeginAssetType(Assets, Asset_Type_Water);
	AddBitmapAsset(Assets, "C:\\Users\\Oskar\\Documents\\GitHub\\game\\Data\\waterTile.bmp");
	EndAssetType(Assets);

	BeginAssetType(Assets, Asset_Type_SlopeLeft);
	AddBitmapAsset(Assets, "C:\\Users\\Oskar\\Documents\\GitHub\\game\\Data\\groundSlope_left.bmp");
	EndAssetType(Assets);

	BeginAssetType(Assets, Asset_Type_SlopeRight);
	AddBitmapAsset(Assets, "C:\\Users\\Oskar\\Documents\\GitHub\\game\\Data\\groundSlope_right.bmp");
	EndAssetType(Assets);

	BeginAssetType(Assets, Asset_Type_Player);
	AddBitmapAsset(Assets, "C:\\Users\\Oskar\\Documents\\GitHub\\game\\Data\\playerBitmap.bmp");
	EndAssetType(Assets);

	//TODO: Bellow are just temporary examples of how arary'd and structured assets would be loaded in.
	/*
		BeginAssetType(Assets, Asset_Type_Stone);
		AddBitmapAsset(Assets, "Stone00.bmp");
		AddBitmapAsset(Assets, "Stone01.bmp");
		AddBitmapAsset(Assets, "Stone02.bmp");
		AddBitmapAsset(Assets, "Stone03.bmp");
		EndAssetType(Assets);
	*/

	return (Assets);
}