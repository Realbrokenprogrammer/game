
om_internal u32
PickBestAsset(game_assets *Assets, asset_type_id TypeID, asset_vector *MatchVector, asset_vector *WeightVector)
{
	u32 Result = 0;
	r32 BestDifference = R32MAX;

	asset_type *Type = Assets->AssetTypes + TypeID;
	for (u32 AssetIndex = Type->FirstAssetIndex; AssetIndex < Type->OnePastLastAssetIndex; ++AssetIndex)
	{
		asset *Asset = Assets->Assets + AssetIndex;

		r32 TotalWeightedDifference = 0.0f;
		for (u32 TagIndex = Asset->GA.FirstTagIndex; TagIndex < Asset->GA.OnePastLastTagIndex; ++TagIndex)
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

struct load_asset_work
{
	task_with_memory *Task;
	asset_slot *Slot;

	platform_file_handle *Handle;
	u64 Offset;
	u64 Size;
	void *Destination;

	asset_state FinalState;
};

om_internal PLATFORM_THREAD_QUEUE_CALLBACK(LoadAssetWork)
{
	load_asset_work *Work = (load_asset_work *)Data;

	Platform.ReadDataFromFile(Work->Handle, Work->Offset, Work->Size, Work->Destination);

	CompletePreviousWritesBeforeFutureWrites;

	// TODO: Should we fill in bs data here if it couldnt read correctly and set it to final state anyway?
	if (PlatformNoFileErrors(Work->Handle))
	{
		Work->Slot->State = Work->FinalState;
	}

	EndTaskWithMemory(Work->Task);
}

inline platform_file_handle *
GetFileHandleFor(game_assets *Assets, u32 FileIndex)
{
	OM_ASSERT(FileIndex < Assets->FileCount);
	platform_file_handle *Result = Assets->Files[FileIndex].Handle;

	return (Result);
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
			asset *Asset = Assets->Assets + ID.Value;
			ga_bitmap *Info = &Asset->GA.Bitmap;
			loaded_bitmap *Bitmap = PushStruct(&Assets->Arena, loaded_bitmap);

			Bitmap->Width = Info->Dimension[0];
			Bitmap->Height = Info->Dimension[1];
			Bitmap->Pitch = 4 * Info->Dimension[0];
			u32 MemorySize = Bitmap->Pitch*Bitmap->Height;
			Bitmap->Pixels = (u32*)((u8 *)PushSize(&Assets->Arena, MemorySize));

			load_asset_work *Work = PushStruct(&Task->Arena, load_asset_work);;
			Work->Task = Task;
			Work->Slot = Assets->Slots + ID.Value;
			Work->Handle = GetFileHandleFor(Assets, Asset->FileIndex);
			Work->Offset = Asset->GA.DataOffset;
			Work->Size = MemorySize;
			Work->Destination = Bitmap->Pixels;
			Work->FinalState = AssetState_Loaded;
			Work->Slot->Bitmap = Bitmap;

			//Bitmap->Pixels = (u32 *)((u8 *)Assets->GAContents + GAAsset->DataOffset);

#if 1
			Bitmap->Pixels = (u32 *)((u8 *)Bitmap->Pixels + Bitmap->Pitch*(Bitmap->Height - 1));
			Bitmap->Pitch = -Bitmap->Pitch;
#endif

			Platform.AddThreadEntry(Assets->TransientState->LowPriorityQueue, LoadAssetWork, Work);
		}
		else
		{
			Assets->Slots[ID.Value].State = AssetState_Unloaded;
		}
	}
}

//struct load_sound_work
//{
//	game_assets *Assets;
//	sound_id ID;
//	task_with_memory *Task;
//	loaded_sound *Sound;
//
//	asset_state FinalState;
//};
//
//om_internal
//PLATFORM_THREAD_QUEUE_CALLBACK(LoadSoundWork)
//{
//	load_sound_work *Work = (load_sound_work *)Data;
//
//	ga_asset *GAAsset = &Work->Assets->Assets[Work->ID.Value];
//	ga_sound *Info = &GAAsset->Sound;
//	loaded_sound *Sound = Work->Sound;
//
//	Sound->SampleCount = Info->SampleCount;
//	Sound->ChannelCount = Info->ChannelCount;
//	OM_ASSERT(Sound->ChannelCount < OM_ARRAYCOUNT(Sound->Samples));
//	
//	u32 SampleDataOffset = GAAsset->DataOffset;
//	for (u32 ChannelIndex = 0; ChannelIndex < Sound->ChannelCount; ++ChannelIndex)
//	{
//		Sound->Samples[ChannelIndex] = (i16 *)(Work->Assets->GAContents + SampleDataOffset);
//		SampleDataOffset += Sound->SampleCount * sizeof(i16);
//	}
//
//	CompletePreviousWritesBeforeFutureWrites;
//
//	Work->Assets->Slots[Work->ID.Value].Sound = Work->Sound;
//	Work->Assets->Slots[Work->ID.Value].State = Work->FinalState;
//
//	EndTaskWithMemory(Work->Task);
//}

om_internal void
LoadSound(game_assets *Assets, sound_id ID)
{
	if (ID.Value &&
		(AtomicCompareExchangeUInt32((u32 *)&Assets->Slots[ID.Value].State, AssetState_Queued, AssetState_Unloaded) == AssetState_Unloaded))
	{
		task_with_memory *Task = BeginTaskWithMemory(Assets->TransientState);
		if (Task)
		{
			//load_sound_work *Work = PushStruct(&Task->Arena, load_sound_work);;
			asset *Asset = Assets->Assets + ID.Value;
			ga_sound *Info = &Asset->GA.Sound;

			loaded_sound *Sound = PushStruct(&Assets->Arena, loaded_sound);
			Sound->SampleCount = Info->SampleCount;
			Sound->ChannelCount = Info->ChannelCount;
			u32 ChannelSize = Sound->SampleCount * sizeof(i16);
			u32 MemorySize = Sound->ChannelCount * ChannelSize;

			void *Memory = PushSize(&Assets->Arena, MemorySize);

			i16 *SoundAt = (i16 *)Memory;
			for (u32 ChannelIndex = 0; ChannelIndex < Sound->ChannelCount; ++ChannelIndex)
			{
				Sound->Samples[ChannelIndex] = SoundAt;
				SoundAt += ChannelSize;
			}

			load_asset_work *Work = PushStruct(&Task->Arena, load_asset_work);
			Work->Task = Task;
			Work->Slot = Assets->Slots + ID.Value;
			Work->Handle = GetFileHandleFor(Assets, Asset->FileIndex);
			Work->Offset = Asset->GA.DataOffset;
			Work->Size = MemorySize;
			Work->Destination = Memory;
			Work->FinalState = AssetState_Loaded;
			Work->Slot->Sound = Sound;

			Platform.AddThreadEntry(Assets->TransientState->LowPriorityQueue, LoadAssetWork, Work);
		}
		else
		{
			Assets->Slots[ID.Value].State = AssetState_Unloaded;
		}
	}
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

	Assets->TagCount = 0;
	Assets->AssetCount = 0;

	{
		platform_file_group FileGroup = Platform.GetAllFilesOfTypeBegin("ga");
		Assets->FileCount = FileGroup.FileCount;
		Assets->Files = PushArray(Arena, Assets->FileCount, asset_file);
		for (u32 FileIndex = 0; FileIndex < Assets->FileCount; ++FileIndex)
		{
			asset_file *File = Assets->Files + FileIndex;

			File->TagBase = Assets->TagCount;


			ZeroStruct(File->Header);
			File->Handle = Platform.OpenFile(FileGroup, FileIndex);
			Platform.ReadDataFromFile(File->Handle, 0, sizeof(File->Header), &File->Header);

			u32 AssetTypeArraySize = File->Header.AssetTypeCount * sizeof(ga_asset_type);
			File->AssetTypeArray = (ga_asset_type *)PushSize(Arena, AssetTypeArraySize);
			Platform.ReadDataFromFile(File->Handle, File->Header.AssetTypesOffset, AssetTypeArraySize, File->AssetTypeArray);

			if (File->Header.MagicValue != GA_MAGIC_VALUE)
			{
				Platform.FileError(File->Handle, "ERROR HERE TODO:::");
			}

			if (File->Header.Version != GA_VERSION)
			{
				Platform.FileError(File->Handle, "ERROR HERE TOODODODO::");
			}

			if (PlatformNoFileErrors(File->Handle))
			{
				Assets->TagCount += File->Header.TagCount;
				Assets->AssetCount += File->Header.AssetCount;
			}
			else
			{
				//TODO: Eventuall notify users of corrupt files?
				InvalidCodePath;
			}
		}
		Platform.GetAllFilesOfTypeEnd(FileGroup);
	}

	// NOTE: Allocate all metadata space here.
	Assets->Assets = PushArray(Arena, Assets->AssetCount, asset);
	Assets->Slots = PushArray(Arena, Assets->AssetCount, asset_slot);
	Assets->Tags = PushArray(Arena, Assets->TagCount, ga_tag);

	// NOTE: Load tags
	for (u32 FileIndex = 0; FileIndex < Assets->FileCount; ++FileIndex)
	{
		asset_file *File = Assets->Files + FileIndex;
		if (PlatformNoFileErrors(File->Handle))
		{
			u32 TagArraySize = sizeof(ga_tag)*File->Header.TagCount;
			Platform.ReadDataFromFile(File->Handle, File->Header.TagsOffset, TagArraySize, Assets->Tags + File->TagBase);
		}
	}

	u32 AssetCount = 0;
	for (u32 DestinationTypeID = 0; DestinationTypeID < Asset_Type_Count; ++DestinationTypeID)
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

					if (SourceType->TypeID == DestinationTypeID)
					{
						u32 AssetCountForType = (SourceType->OnePastLastAssetIndex - SourceType->FirstAssetIndex);

						temporary_memory TemporaryMemory = CreateTemporaryMemory(&TransientState->TransientArena);
						ga_asset *GAAssetArray = PushArray(&TransientState->TransientArena, AssetCountForType, ga_asset);

						Platform.ReadDataFromFile(File->Handle, 
												  File->Header.AssetsOffset + 
												  SourceType->FirstAssetIndex * sizeof(ga_asset), 
												  AssetCountForType * sizeof(ga_asset), 
												  GAAssetArray);
						
						for (u32 AssetIndex = 0; AssetIndex < AssetCountForType; ++AssetIndex)
						{
							ga_asset *GAAsset = GAAssetArray + AssetIndex;

							OM_ASSERT(AssetCount < Assets->AssetCount);
							asset *Asset = Assets->Assets + AssetCount++;

							Asset->FileIndex = FileIndex;
							Asset->GA = *GAAsset;
							Asset->GA.FirstTagIndex += File->TagBase;
							Asset->GA.OnePastLastTagIndex += File->TagBase;
						}

						DestroyTemporaryMemory(TemporaryMemory);
					}
				}
			}
		}

		DestinationType->OnePastLastAssetIndex = AssetCount;
	}

	OM_ASSERT(AssetCount == Assets->AssetCount);

#if 0
	debug_read_file_result ReadResult = Platform.DEBUGReadEntireFile("C:\\Users\\Oskar\\Documents\\GitHub\\game\\Data\\test.ga");
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
#endif

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