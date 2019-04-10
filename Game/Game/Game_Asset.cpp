
om_internal u32
PickBestAsset(game_assets *Assets, asset_type_id TypeID, asset_vector *MatchVector, asset_vector *WeightVector)
{
	u32 Result = 0;
	r32 BestDifference = R32MAX;

	asset_type *Type = Assets->AssetTypes + TypeID;
	for (u32 AssetIndex = Type->FirstAssetIndex; AssetIndex < Type->OnePastLastAssetIndex; ++AssetIndex)
	{
		ga_asset *Asset = Assets->Assets + AssetIndex;

		r32 TotalWeightedDifference = 0.0f;
		for (u32 TagIndex = Asset->FirstTagIndex; TagIndex < Asset->OnePastLastTagIndex; ++TagIndex)
		{
			ga_tag *Tag = Assets->Tags + TagIndex;
			
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
			Result = AssetIndex;
		}
	}

	return (Result);
}

om_internal u32
GetFirstAssetID(game_assets *Assets, asset_type_id TypeID)
{
	u32 Result = 0;

	asset_type *Type = Assets->AssetTypes + TypeID;
	if (Type->FirstAssetIndex != Type->OnePastLastAssetIndex)
	{
		Result = Type->FirstAssetIndex;
	}

	return (Result);
}

//inline bitmap_id
//PickBestBitmap(game_assets *Assets, asset_type_id TypeID, asset_vector *MatchVector, asset_vector *WeightVector)
//{
//	bitmap_id Result = { PickBestAsset(Assets, TypeID, MatchVector, WeightVector) };
//
//	return (Result);
//}

inline bitmap_id
GetFirstBitmapID(game_assets *Assets, asset_type_id TypeID)
{
	bitmap_id Result = { GetFirstAssetID(Assets, TypeID) };

	return (Result);
}

//inline sound_id
//PickBestSound(game_assets *Assets, asset_type_id TypeID, asset_vector *MatchVector, asset_vector *WeightVector)
//{
//	sound_id Result = { PickBestAsset(Assets, TypeID, MatchVector, WeightVector) };
//
//	return (Result);
//}

inline sound_id
GetFirstSoundID(game_assets *Assets, asset_type_id TypeID)
{
	sound_id Result = { GetFirstAssetID(Assets, TypeID) };

	return (Result);
}

#if 0

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
DEBUGLoadWAV(char *FileName, u32 SectionFirstSampleIndex, u32 SectionSampleCount)
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
		u32 SampleCount = SampleDataSize / (ChannelCount * sizeof(i16));

		if (ChannelCount == 1)
		{
			Result.Samples[0] = SampleData;
			Result.Samples[1] = 0;
		}
		else if (ChannelCount == 2)
		{
			Result.Samples[0] = SampleData;
			Result.Samples[1] = SampleData + SampleCount;

			for (u32 SampleIndex = 0; SampleIndex < SampleCount; ++SampleIndex)
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
		b32 AtEnd = true;
		Result.ChannelCount = 1;

		if (SectionSampleCount)
		{
			OM_ASSERT((SectionFirstSampleIndex + SectionSampleCount) <= SampleCount);
			AtEnd = ((SectionFirstSampleIndex + SectionSampleCount) == SampleCount);
			SampleCount = SectionSampleCount;

			for (u32 ChannelIndex = 0; ChannelIndex < Result.ChannelCount; ++ChannelIndex)
			{
				Result.Samples[ChannelIndex] += SectionFirstSampleIndex;
			}
		}

		if (AtEnd)
		{
			for (u32 ChannelIndex = 0; ChannelIndex < Result.ChannelCount; ++ChannelIndex)
			{
				for (u32 SampleIndex = SampleCount; SampleIndex < (SampleCount + 8); ++SampleIndex)
				{
					Result.Samples[ChannelIndex][SampleIndex] = 0;
				}
			}
		}

		Result.SampleCount = SampleCount;
	}

	return (Result);
}

#endif

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

	ga_asset *GAAsset = &Work->Assets->Assets[Work->ID.Value];
	ga_bitmap *Info = &GAAsset->Bitmap;
	loaded_bitmap *Bitmap = Work->Bitmap;

	Bitmap->Width = Info->Dimension[0];
	Bitmap->Height = Info->Dimension[1];
	Bitmap->Pitch = 4 * Info->Dimension[0];
	Bitmap->Pixels = (u32 *)((u8 *)Work->Assets->GAContents + GAAsset->DataOffset);

#if 1
	Bitmap->Pixels = (u32 *)((u8 *)Bitmap->Pixels + Bitmap->Pitch*(Bitmap->Height - 1));
	Bitmap->Pitch = -Bitmap->Pitch;
#endif

	CompletePreviousWritesBeforeFutureWrites;

	Work->Assets->Slots[Work->ID.Value].Bitmap = Work->Bitmap;
	Work->Assets->Slots[Work->ID.Value].State = Work->FinalState;

	EndTaskWithMemory(Work->Task);
}

om_internal void
LoadBitmap(game_assets *Assets, bitmap_id ID)
{
	if (ID.Value && 
		(AtomicCompareExchangeUInt32((u32 *)&Assets->Slots[ID.Value].State, AssetState_Queued, AssetState_Unloaded) == AssetState_Unloaded))
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
		else
		{
			Assets->Slots[ID.Value].State = AssetState_Unloaded;
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

	ga_asset *GAAsset = &Work->Assets->Assets[Work->ID.Value];
	ga_sound *Info = &GAAsset->Sound;
	loaded_sound *Sound = Work->Sound;

	Sound->SampleCount = Info->SampleCount;
	Sound->ChannelCount = Info->ChannelCount;
	OM_ASSERT(Sound->ChannelCount < OM_ARRAYCOUNT(Sound->Samples));
	
	u32 SampleDataOffset = GAAsset->DataOffset;
	for (u32 ChannelIndex = 0; ChannelIndex < Sound->ChannelCount; ++ChannelIndex)
	{
		Sound->Samples[ChannelIndex] = (i16 *)(Work->Assets->GAContents + SampleDataOffset);
		SampleDataOffset += Sound->SampleCount * sizeof(i16);
	}

	CompletePreviousWritesBeforeFutureWrites;

	Work->Assets->Slots[Work->ID.Value].Sound = Work->Sound;
	Work->Assets->Slots[Work->ID.Value].State = Work->FinalState;

	EndTaskWithMemory(Work->Task);
}

om_internal void
LoadSound(game_assets *Assets, sound_id ID)
{
	if (ID.Value &&
		(AtomicCompareExchangeUInt32((u32 *)&Assets->Slots[ID.Value].State, AssetState_Queued, AssetState_Unloaded) == AssetState_Unloaded))
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
		else
		{
			Assets->Slots[ID.Value].State = AssetState_Unloaded;
		}
	}
}

#if 0

om_internal void
BeginAssetType(game_assets *Assets, asset_type_id TypeID)
{
	OM_ASSERT(Assets->DEBUGAssetType == 0);

	Assets->DEBUGAssetType = Assets->AssetTypes + TypeID;
	Assets->DEBUGAssetType->FirstAssetIndex = Assets->DEBUGUsedAssetCount;
	Assets->DEBUGAssetType->OnePastLastAssetIndex = Assets->DEBUGAssetType->FirstAssetIndex;
}

om_internal bitmap_id
AddBitmapAsset(game_assets *Assets, char *FileName)
{
	OM_ASSERT(Assets->DEBUGAssetType);
	OM_ASSERT(Assets->DEBUGAssetType->OnePastLastAssetIndex < Assets->AssetCount);

	bitmap_id Result = { Assets->DEBUGAssetType->OnePastLastAssetIndex++ };
	asset *Asset = Assets->Assets + Result.Value;
	Asset->FirstTagIndex = Assets->DEBUGUsedTagCount;
	Asset->OnePastLastTagIndex = Asset->FirstTagIndex;
	Asset->Bitmap.FileName = PushString(&Assets->Arena, FileName);

	Assets->DEBUGAsset = Asset;

	return (Result);
}

om_internal sound_id
AddSoundAsset(game_assets *Assets, char *FileName, u32 FirstSampleIndex = 0, u32 SampleCount = 0)
{
	OM_ASSERT(Assets->DEBUGAssetType);
	OM_ASSERT(Assets->DEBUGAssetType->OnePastLastAssetIndex < Assets->AssetCount);

	sound_id Result = { Assets->DEBUGAssetType->OnePastLastAssetIndex++ };
	asset *Asset = Assets->Assets + Result.Value;
	Asset->FirstTagIndex = Assets->DEBUGUsedTagCount;
	Asset->OnePastLastTagIndex = Asset->FirstTagIndex;
	Asset->Sound.FileName = PushString(&Assets->Arena, FileName);
	Asset->Sound.FirstSampleIndex = FirstSampleIndex;
	Asset->Sound.SampleCount = SampleCount;
	Asset->Sound.NextIDToPlay.Value = 0;

	Assets->DEBUGAsset = Asset;

	return (Result);
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

#endif

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

	Assets->TagCount = 0;
	Assets->AssetCount = 0;

#if 0
	{
		platform_file_group FileGroup = PlatformGetAllFilesOfTypeBegin(".ga");
		Assets->FileCount = FileGroup.FileCount;
		Assets->Files = PushArray(Arena, Assets->FileCount, asset_file);
		for (u32 FileIndex = 0; FileIndex < Assets->FileCount; ++FileIndex)
		{
			asset_file *File = Assets->Files + FileIndex;

			u32 AssetTypeArraySize = File->Header.AssetTypeCount * sizeof(ga_asset_type);

			ZeroStruct(File->Header);
			File->Handle  PlatformOpenFile(FileGroup, FileIndex);
			PlatformReadDataFromFile(File->Handle, 0, sizeof(File->Header), &File->Header);
			File->AssetTypeArray = (ga_asset_type *)PushSize(Arena, AssetTypeArraySize);
			PlatformReadDataFromFile(File->Handle, File->Header.AssetTypes, AssetTypeArraySize, File->AssetTypeArray);

			if (Header->MagicValue != GA_MAGIC_VALUE)
			{
				PlatformFileError(File->Handle, "ERROR HERE TODO:::");
			}

			if (Header->Version != GA_VERSION)
			{
				PlatformFileError(File->Handle, "ERROR HERE TOODODODO::");
			}

			if (PlatformNoFileErrors(File->Handle))
			{
				Assets->TagCount += Header->TagCount;
				Assets->AssetCount += Header->AssetCount;
			}
			else
			{
				//TODO: Eventuall notify users of corrupt files?
				InvalidCodePath;
			}
		}
		PlatformGetAllFilesOfTypeEnd(FileGroup);
	}

	Assets->Assets = PushArray(Arena, Assets->AssetCount, ga_asset);
	Assets->Slots = PushArray(Arena, Assets->AssetCount, asset_slot);
	Assets->Tags = PushArray(Arena, Assets->TagCount, ga_asset);

	u32 AssetCount = 0;
	u32 TagCount = 0;
	for (u32 DestinationTypeID = 0; DestinationTypeID < AssetCount; ++DestinationTypeID)
	{
		asset_type *DestinationType = Assets->AssetTypes + DestinationTypeID;
		DestinationType->FirstAssetIndex = AssetCount;

		for (u32 FileIndex = 0; FileIndex < Assets->FileCount; ++FileIndex)
		{
			asset_file *File = Assets->Files + FileIndex;
			if (PlatformNoFileErrors(File->Handle))
			{
				for (u32 SourceIndex = 0; SourceIndex < File->Header.AssetTypeCount; ++SourceIndex)
				{
					ga_asset_type *SourceType = File->AssetTypeArray + SourceIndex;

					if (SourceType->TypeID == AssetTypeID)
					{
						PlatformReadDataFromFile();
						AssetCount += ;
					}
				}
			}
		}

		DestinationType->OnePastLastAssetIndex = AssetCount;
	}

	OM_ASSERT(AssetCount == Assets->AssetCount);
	OM_ASSERT(TagCount == Assets->TagCount);
#endif

	debug_read_file_result ReadResult = DEBUGPlatformReadEntireFile("C:\\Users\\Oskar\\Documents\\GitHub\\game\\Data\\test.ga");
	if (ReadResult.ContentsSize != 0)
	{
		ga_header *Header = (ga_header *)ReadResult.Contents;
		
		Assets->AssetCount = Header->AssetCount;
		Assets->Assets = (ga_asset *)((u8 *)ReadResult.Contents + Header->AssetsOffset);
		Assets->Slots = PushArray(Arena, Assets->AssetCount, asset_slot);
		
		Assets->TagCount = Header->TagCount;
		Assets->Tags = (ga_tag *)((u8 *)ReadResult.Contents + Header->TagsOffset);

		ga_asset_type *GAAssetTypes = (ga_asset_type *)((u8 *)ReadResult.Contents + Header->AssetTypesOffset);
		
		for (u32 Index = 0; Index < Header->AssetTypeCount; ++Index)
		{
			ga_asset_type *Source = GAAssetTypes + Index;

			if (Source->TypeID < Asset_Type_Count)
			{
				asset_type *Destination = Assets->AssetTypes + Source->TypeID;

				//TODO: Support for merging.
				OM_ASSERT(Destination->FirstAssetIndex == 0);
				OM_ASSERT(Destination->OnePastLastAssetIndex == 0);

				Destination->FirstAssetIndex = Source->FirstAssetIndex;
				Destination->OnePastLastAssetIndex = Source->OnePastLastAssetIndex;
			}
		}

		Assets->GAContents = (u8 *)ReadResult.Contents;
	}

#if 0

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

	//TODO: This is temporary code for testing. Should be removed once we load this in from asset files.
	u32 SingleMusicChunk = 10 * 48000;
	u32 TotalMusicSampleCount = 7468095;

	BeginAssetType(Assets, Asset_Type_Music);
	sound_id LastMusic = { 0 };
	for (u32 FirstSampleIndex = 0; FirstSampleIndex < TotalMusicSampleCount; FirstSampleIndex += SingleMusicChunk)
	{
		u32 SampleCount = TotalMusicSampleCount - FirstSampleIndex;
		if (SampleCount > SingleMusicChunk)
		{
			SampleCount = SingleMusicChunk;
		}
		sound_id ThisMusic = AddSoundAsset(Assets, "C:\\Users\\Oskar\\Documents\\GitHub\\game\\Data\\music_test.wav", FirstSampleIndex, SampleCount);
		if (IsValid(LastMusic))
		{
			Assets->Assets[LastMusic.Value].Sound.NextIDToPlay= ThisMusic;
		}
		LastMusic = ThisMusic;
	}
	EndAssetType(Assets);
#endif
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