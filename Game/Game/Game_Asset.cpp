
om_internal bitmap_id
PickBestAsset(game_assets *Assets, asset_type_id TypeID, asset_vector *MatchVector, asset_vector *WeightVector)
{
	bitmap_id Result = {};
	r32 BestDifference = R32MAX;

	asset_type *Type = Assets->AssetTypes + TypeID;
	for (u32 AssetIndex = Type->FirstAssetIndex; AssetIndex < Type->OnePastLastAssetIndex; ++AssetIndex)
	{
		asset *Asset = Assets->Assets + AssetIndex;

		r32 TotalWeightedDifference = 0.0f;
		for (u32 TagIndex = Asset->FirstTagIndex; TagIndex < Asset->OnePastLastTagIndex; ++TagIndex)
		{
			asset_tag *Tag = Assets->Tags + TagIndex;
			
			r32 A = MatchVector->E[Tag->ID];
			r32 B = Tag->Value;
			r32 D0 = AbsoluteValue(A - B);
			r32 D1 = AbsoluteValue((A - Assets->TagRange[Tag->ID] * SignOf(A)) - B);
			r32 Difference = OM_MIN(D0, D1);

			r32 Weighted = WeightVector->E[Tag->ID] * Difference;
			TotalWeightedDifference += Weighted;
		}

		if (BestDifference > TotalWeightedDifference)
		{
			BestDifference = TotalWeightedDifference;
			Result.Value = Asset->SlotID;
		}
	}

	return (Result);
}

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

//Note: Source for WAVE header: http://www-mmsp.ece.mcgill.ca/Documents/AudioFormats/WAVE/WAVE.html
struct WAVE_header
{
	u32 RIFFID;
	u32 Size;
	u32 WAVEID;
};

#define RIFF_CODE(a, b, c, d) (((u32)(a) << 0) | ((u32)(b) << 8) | ((u32)(c) << 16) | ((u32)(d) << 24))

enum
{
	WAVE_ChunkID_fmt = RIFF_CODE('f', 'm', 't', ' '),
	WAVE_ChunkID_data = RIFF_CODE('d', 'a', 't', 'a'),
	WAVE_ChunkID_RIFF = RIFF_CODE('R', 'I', 'F', 'F'),
	WAVE_ChunkID_WAVE = RIFF_CODE('W', 'A', 'V', 'E')
};

struct WAVE_chunk
{
	u32 ID;
	u32 Size;
};

struct WAVE_fmt {
	u16 wFormatTag;
	u16 nChannels;
	u32 nSamplesPerSec;
	u32 nAvgBytesPerSec;
	u16 nBlockAlign;
	u16 wBitsPerSample;
	u16 cbSize;
	u16 wValidBitsPerSample;
	u32 dwChannelMask;
	u8 SubFormat[16];
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

struct riff_iterator
{
	u8 *At;
	u8 *Stop;
};

inline riff_iterator
ParseChunkAt(void *At, void *Stop)
{
	riff_iterator Iterator = {};

	Iterator.At = (u8 *)At;
	Iterator.Stop = (u8 *)Stop;

	return (Iterator);
}

inline riff_iterator
NextChunk(riff_iterator Iterator)
{
	WAVE_chunk *Chunk = (WAVE_chunk *)Iterator.At;
	u32 Size = (Chunk->Size + 1) & ~1;
	Iterator.At += sizeof(WAVE_chunk) + Size;

	return (Iterator);
}

inline b32
IsValid(riff_iterator Iterator)
{
	b32 Result = (Iterator.At < Iterator.Stop);

	return (Result);
}

inline void *
GetChunkData(riff_iterator Iterator)
{
	void *Result = (Iterator.At + sizeof(WAVE_chunk));

	return (Result);
}

inline u32
GetType(riff_iterator Iterator)
{
	WAVE_chunk *Chunk = (WAVE_chunk *)Iterator.At;
	u32 Result = Chunk->ID;

	return (Result);
}

inline u32
GetChunkDataSize(riff_iterator Iterator)
{
	WAVE_chunk *Chunk = (WAVE_chunk *)Iterator.At;
	u32 Result = Chunk->Size;

	return (Result);
}

om_internal loaded_sound
DEBUGLoadWAV(char *FileName)
{
	loaded_sound Result = {};

	debug_read_file_result ReadResult = DEBUGPlatformReadEntireFile(FileName);
	if (ReadResult.ContentsSize != 0)
	{
		WAVE_header *Header = (WAVE_header *)ReadResult.Contents;

		OM_ASSERT(Header->RIFFID == WAVE_ChunkID_RIFF);
		OM_ASSERT(Header->WAVEID == WAVE_ChunkID_WAVE);

		u32 ChannelCount = 0;
		u32 SampleDataSize = 0;
		i16 *SampleData = 0;

		for (riff_iterator Iterator = ParseChunkAt(Header + 1, (u8 *)(Header + 1) + Header->Size - 4); IsValid(Iterator); Iterator = NextChunk(Iterator))
		{
			switch (GetType(Iterator))
			{
				case WAVE_ChunkID_fmt:
				{
					WAVE_fmt *fmt = (WAVE_fmt *)GetChunkData(Iterator);
					OM_ASSERT(fmt->wFormatTag == 1); //Note: We only support PCM.
					OM_ASSERT(fmt->nSamplesPerSec == 48000);
					OM_ASSERT(fmt->wBitsPerSample == 16);
					OM_ASSERT(fmt->nBlockAlign == (sizeof(int16)*fmt->nChannels));
					ChannelCount = fmt->nChannels;
				} break;

				case WAVE_ChunkID_data:
				{
					SampleData = (i16 *)GetChunkData(Iterator);
					SampleDataSize = GetChunkDataSize(Iterator);
				} break;
			}
		}

		OM_ASSERT(ChannelCount && SampleData);

		Result.ChannelCount = ChannelCount;
		Result.SampleCount = SampleDataSize / (ChannelCount * sizeof(i16));

		if (ChannelCount == 1)
		{
			Result.Samples[0] = SampleData;
			Result.Samples[1] = 0;
		}
		else if (ChannelCount == 2)
		{
			Result.Samples[0] = SampleData;
			Result.Samples[1] = SampleData + Result.SampleCount;

			for (u32 SampleIndex = 0; SampleIndex < Result.SampleCount; ++SampleIndex)
			{
				i16 Source = SampleData[2 * SampleIndex];
				SampleData[2 * SampleIndex] = SampleData[SampleIndex];
				SampleData[SampleIndex] = Source;
			}
		}
		else
		{
			OM_ASSERT(!"Invalid number of channels in WAV file.");
		}

		// TODO: Load the right channels.
		Result.ChannelCount = 1;
	}

	return (Result);
}

struct load_bitmap_work
{
	game_assets *Assets;
	bitmap_id ID;
	task_with_memory *Task;
	loaded_bitmap *Bitmap;

	asset_state FinalState;
};
om_internal
PLATFORM_THREAD_QUEUE_CALLBACK(LoadBitmapWork)
{
	load_bitmap_work *Work = (load_bitmap_work *)Data;

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
			load_bitmap_work *Work = PushStruct(&Task->Arena, load_bitmap_work);;
			Work->Assets = Assets;
			Work->ID = ID;
			Work->Task = Task;
			Work->Bitmap = PushStruct(&Assets->Arena, loaded_bitmap);
			Work->FinalState = AssetState_Loaded;

			PlatformAddThreadEntry(Assets->TransientState->LowPriorityQueue, LoadBitmapWork, Work);
		}
	}
}

struct load_sound_work
{
	game_assets *Assets;
	sound_id ID;
	task_with_memory *Task;
	loaded_sound *Sound;

	asset_state FinalState;
};

om_internal
PLATFORM_THREAD_QUEUE_CALLBACK(LoadSoundWork)
{
	load_sound_work *Work = (load_sound_work *)Data;

	asset_sound_info *Info = Work->Assets->SoundInfos + Work->ID.Value;
	*Work->Sound = DEBUGLoadWAV(Info->FileName);

	CompletePreviousWritesBeforeFutureWrites;

	Work->Assets->Sounds[Work->ID.Value].Sound = Work->Sound;
	Work->Assets->Sounds[Work->ID.Value].State = Work->FinalState;

	EndTaskWithMemory(Work->Task);
}

om_internal void
LoadSound(game_assets *Assets, sound_id ID)
{
	if (ID.Value &&
		(AtomicCompareExchangeUInt32((u32 *)&Assets->Sounds[ID.Value].State, AssetState_Unloaded, AssetState_Queued) == AssetState_Unloaded))
	{
		task_with_memory *Task = BeginTaskWithMemory(Assets->TransientState);
		if (Task)
		{
			load_sound_work *Work = PushStruct(&Task->Arena, load_sound_work);;
			Work->Assets = Assets;
			Work->ID = ID;
			Work->Task = Task;
			Work->Sound = PushStruct(&Assets->Arena, loaded_sound);
			Work->FinalState = AssetState_Loaded;

			PlatformAddThreadEntry(Assets->TransientState->LowPriorityQueue, LoadSoundWork, Work);
		}
	}
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
	OM_ASSERT(Assets->DEBUGAssetType->OnePastLastAssetIndex < Assets->AssetCount);

	asset *Asset = Assets->Assets + Assets->DEBUGAssetType->OnePastLastAssetIndex++;
	Asset->FirstTagIndex = Assets->DEBUGUsedTagCount;
	Asset->OnePastLastTagIndex = Asset->FirstTagIndex;
	Asset->SlotID = DEBUGAddBitmapInfo(Assets, FileName).Value;

	Assets->DEBUGAsset = Asset;
}

om_internal void
AddAssetTag(game_assets *Assets, asset_tag_id ID, r32 Value)
{
	OM_ASSERT(Assets->DEBUGAsset);

	++Assets->DEBUGAsset->OnePastLastTagIndex;
	asset_tag *Tag = Assets->Tags + Assets->DEBUGUsedTagCount++;
	
	Tag->ID = ID;
	Tag->Value = Value;
}

om_internal void
EndAssetType(game_assets *Assets)
{
	OM_ASSERT(Assets->DEBUGAssetType);
	Assets->DEBUGUsedAssetCount = Assets->DEBUGAssetType->OnePastLastAssetIndex;
	Assets->DEBUGAssetType = 0;
	Assets->DEBUGAsset = 0;
}

om_internal game_assets *
CreateGameAssets(memory_arena *Arena, memory_index Size, transient_state *TransientState)
{
	game_assets *Assets = PushStruct(Arena, game_assets);
	CreateSubArena(&Assets->Arena, Arena, Size);
	Assets->TransientState = TransientState;

	for (u32 TagType = 0; TagType < Asset_Tag_Count; ++TagType)
	{
		Assets->TagRange[TagType] = 1000000.0f;
	}

	//TODO: Example of how to set a tag range for specific tag:
	//Assets->TagRange[Asset_Tag_PlayerFacingDirection] = Tau32;

	Assets->BitmapCount = 256*Asset_Type_Count; //TODO: Temporary large value for debugging.
	Assets->BitmapInfos = PushArray(Arena, Assets->BitmapCount, asset_bitmap_info);
	Assets->Bitmaps = PushArray(Arena, Assets->BitmapCount, asset_slot);

	//TODO: There is currently no sounds so this is just temporary.
	Assets->SoundCount = 1;
	Assets->Sounds = PushArray(Arena, Assets->SoundCount, asset_slot);

	Assets->TagCount = 1024 * Asset_Type_Count;
	Assets->Tags = PushArray(Arena, Assets->TagCount, asset_tag);

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

	/*
		//TODO: Bellow are just temporary examples of how arary'd and structured assets would be loaded in.
		BeginAssetType(Assets, Asset_Type_Stone);
		AddBitmapAsset(Assets, "Stone00.bmp");
		AddBitmapAsset(Assets, "Stone01.bmp");
		AddBitmapAsset(Assets, "Stone02.bmp");
		AddBitmapAsset(Assets, "Stone03.bmp");
		EndAssetType(Assets);

		//TODO: Bellow are temporary example of how a structured asset with tags would be loaded in.
		real32 AngleRight = 0.0f*Tau32;
		real32 AngleBack = 0.25f*Tau32;
		real32 AngleLeft = 0.5f*Tau32;
		real32 AngleFront = 0.75f*Tau32;


		BeginAssetType(Assets, Asset_Head);
		AddBitmapAsset(Assets, "player_right_head.bmp");
		AddTag(Assets, Tag_FacingDirection, AngleRight);
		AddBitmapAsset(Assets, "player_back_head.bmp");
		AddTag(Assets, Tag_FacingDirection, AngleBack);
		AddBitmapAsset(Assets, "player_left_head.bmp");
		AddTag(Assets, Tag_FacingDirection, AngleLeft);
		AddBitmapAsset(Assets, "player_front_head.bmp");
		AddTag(Assets, Tag_FacingDirection, AngleFront);
		EndAssetType(Assets);
	*/

	return (Assets);
}