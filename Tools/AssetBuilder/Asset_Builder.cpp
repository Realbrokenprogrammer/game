#include "Asset_Builder.h"

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

struct read_file_result
{
    u32 ContentsSize;
    void *Contents;
};

read_file_result
ReadEntireFile(char *FileName)
{
    read_file_result Result = {};

    FILE *In = fopen(FileName, "rb");

    if (In)
    {
        fseek(In, 0, SEEK_END);
        Result.ContentsSize = ftell(In);
        fseek(In, 0, SEEK_SET);

        Result.Contents = malloc(Result.ContentsSize);
        fread(Result.Contents, Result.ContentsSize, 1, In);

        fclose(In);
    }
    else
    {
        printf("ERROR: Cannot open file: %s.\n", FileName);
    }

    return (Result);
}

//Note: This is not complete bitmap loading code hence it should only be used as such.
om_internal loaded_bitmap
LoadBitmap(char *FileName)
{
	loaded_bitmap Result = {};

	read_file_result ReadResult = ReadEntireFile(FileName);
	if (ReadResult.ContentsSize != 0)
	{
        Result.Free = ReadResult.Contents;

		bitmap_header *Header = (bitmap_header *)ReadResult.Contents;

		u32 *Pixels = (u32 *)((u8 *)ReadResult.Contents + Header->BitmapOffset);

		Result.Memory = Pixels;
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
#if 0
	Result.Memory = (u32 *)((u8 *)Result.Memory + Result.Pitch*(Result.Height - 1));
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
LoadWAV(char *FileName, u32 SectionFirstSampleIndex, u32 SectionSampleCount)
{
	loaded_sound Result = {};

	read_file_result ReadResult = ReadEntireFile(FileName);
	if (ReadResult.ContentsSize != 0)
	{
        Result.Free = ReadResult.Contents;

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

om_internal loaded_bitmap
LoadGlyphBitmap(char *FileName, u32 Codepoint)
{
    loaded_bitmap Result = {};

    read_file_result TTFFile = ReadEntireFile(FileName);
    if (TTFFile.ContentsSize != 0)
    {
        stbtt_fontinfo Font;
        stbtt_InitFont(&Font, (u8 *)TTFFile.Contents, stbtt_GetFontOffsetForIndex((u8 *)TTFFile.Contents, 0));

        int Width, Height, XOffset, YOffset;
        u8 *MonoBitmap = stbtt_GetCodepointBitmap(&Font, 0, stbtt_ScaleForPixelHeight(&Font, 128.0f), 
                                                  Codepoint, &Width, &Height, &XOffset, &YOffset); 

        Result.Width = Width;
        Result.Height = Height; 
        Result.Pitch = Result.Width * BITMAP_BYTES_PER_PIXEL;
        Result.Memory = malloc(Height*Result.Pitch);
        Result.Free = Result.Memory;

        u8 *Source = MonoBitmap;
        u8 *DestinationRow = (u8 *)Result.Memory + (Height - 1)*Result.Pitch;
        for (s32 Y = 0; Y < Height; ++Y)
        {
            u32 *Destination = (u32 *)DestinationRow;
            for (s32 X = 0; X < Width; ++X)
            {
                u8 Alpha = *Source++;
                *Destination++ = ((Alpha << 24)|
                                  (Alpha << 16)|
                                  (Alpha << 8) |
                                  (Alpha << 0));
            }

            DestinationRow -= Result.Pitch;
        }

        stbtt_FreeBitmap(MonoBitmap, 0);
        free(TTFFile.Contents);
    }

    return (Result);
}

om_internal void
BeginAssetType(game_assets *Assets, asset_type_id TypeID)
{
	OM_ASSERT(Assets->DEBUGAssetType == 0);

	Assets->DEBUGAssetType = Assets->AssetTypes + TypeID;
    Assets->DEBUGAssetType->TypeID = TypeID;
	Assets->DEBUGAssetType->FirstAssetIndex = Assets->AssetCount;
	Assets->DEBUGAssetType->OnePastLastAssetIndex = Assets->DEBUGAssetType->FirstAssetIndex;
}

om_internal bitmap_id
AddBitmapAsset(game_assets *Assets, char *FileName)
{
	OM_ASSERT(Assets->DEBUGAssetType);
	OM_ASSERT(Assets->DEBUGAssetType->OnePastLastAssetIndex < OM_ARRAYCOUNT(Assets->Assets));

	bitmap_id Result = { Assets->DEBUGAssetType->OnePastLastAssetIndex++ };
	asset_source *Source = Assets->AssetSources + Result.Value;
	ga_asset *GA = Assets->Assets + Result.Value;
	GA->FirstTagIndex = Assets->TagCount;
	GA->OnePastLastTagIndex = GA->FirstTagIndex;

    Source->Type = AssetType_Bitmap;
    Source->FileName = FileName;

    Assets->AssetIndex = Result.Value;

	return (Result);
}

om_internal bitmap_id
AddCharacterAsset(game_assets *Assets, char *FontFile, u32 Codepoint)
{
    OM_ASSERT(Assets->DEBUGAssetType);
    OM_ASSERT(Assets->DEBUGAssetType->OnePastLastAssetIndex < OM_ARRAYCOUNT(Assets->Assets));

    bitmap_id Result = { Assets->DEBUGAssetType->OnePastLastAssetIndex++ };
    asset_source *Source = Assets->AssetSources + Result.Value;
    ga_asset *GA = Assets->Assets + Result.Value;
    GA->FirstTagIndex = Assets->TagCount;
    GA->OnePastLastTagIndex = GA->FirstTagIndex;

    Source->Type = AssetType_Font;
    Source->FileName = FontFile;
    Source->Codepoint = Codepoint;

    Assets->AssetIndex = Result.Value;

    return (Result);
}

om_internal sound_id
AddSoundAsset(game_assets *Assets, char *FileName, u32 FirstSampleIndex = 0, u32 SampleCount = 0)
{
    OM_ASSERT(Assets->DEBUGAssetType);
    OM_ASSERT(Assets->DEBUGAssetType->OnePastLastAssetIndex < OM_ARRAYCOUNT(Assets->Assets));

    sound_id Result = { Assets->DEBUGAssetType->OnePastLastAssetIndex++ };
    asset_source *Source = Assets->AssetSources + Result.Value;
	ga_asset *GA = Assets->Assets + Result.Value;
	GA->FirstTagIndex = Assets->TagCount;
    GA->OnePastLastTagIndex = GA->FirstTagIndex;
    GA->Sound.SampleCount = SampleCount;
    GA->Sound.Chain = GASoundChain_None;

    Source->Type = AssetType_Sound;
    Source->FileName = FileName;
    Source->FirstSampleIndex = FirstSampleIndex;

    Assets->AssetIndex = Result.Value;

    return (Result);
}

om_internal void
AddAssetTag(game_assets *Assets, asset_tag_id ID, r32 Value)
{
    OM_ASSERT(Assets->AssetIndex);

    ga_asset *GA = Assets->Assets + Assets->AssetIndex;
    ++GA->OnePastLastTagIndex;
    ga_tag *Tag = Assets->Tags + Assets->TagCount++;

    Tag->ID = ID;
    Tag->Value = Value;
}

om_internal void
EndAssetType(game_assets *Assets)
{
    OM_ASSERT(Assets->DEBUGAssetType);
    Assets->AssetCount = Assets->DEBUGAssetType->OnePastLastAssetIndex;
    Assets->DEBUGAssetType = 0;
    Assets->AssetIndex = 0;
}

om_internal void
WriteGA(game_assets *Assets, char *FileName)
{
    FILE *Out = 0;

    Out = fopen(FileName, "wb");
    if (Out)
    {
        ga_header Header = {};
        Header.MagicValue = GA_MAGIC_VALUE;
        Header.Version = GA_VERSION;
        Header.TagCount = Assets->TagCount;
        Header.AssetTypeCount = Asset_Type_Count; //TODO: We might not want to do this.
        Header.AssetCount = Assets->AssetCount;

        u32 TagArraySize= Header.TagCount * sizeof(ga_tag);
        u32 AssetTypeArraySize = Header.AssetTypeCount * sizeof(ga_asset_type);
        u32 AssetArraySize = Header.AssetCount * sizeof(ga_asset);

        Header.TagsOffset = sizeof(Header);
        Header.AssetTypesOffset = Header.TagsOffset + TagArraySize;
        Header.AssetsOffset = Header.AssetTypesOffset + AssetTypeArraySize;

        fwrite(&Header, sizeof(Header), 1, Out);
        fwrite(Assets->Tags, TagArraySize, 1, Out);
        fwrite(Assets->AssetTypes, AssetTypeArraySize, 1, Out);
        
        fseek(Out, AssetArraySize, SEEK_CUR);
        for (u32 AssetIndex = 1; AssetIndex < Header.AssetCount; ++AssetIndex)
        {
            asset_source *Source = Assets->AssetSources + AssetIndex;
            ga_asset *Destination = Assets->Assets + AssetIndex;
            Destination->DataOffset = ftell(Out);

            if (Source->Type == AssetType_Sound)
            {
                loaded_sound Sound = LoadWAV(Source->FileName, Source->FirstSampleIndex, Destination->Sound.SampleCount);

                Destination->Sound.SampleCount = Sound.SampleCount;
                Destination->Sound.ChannelCount = Sound.ChannelCount;

                for (u32 ChannelIndex = 0; ChannelIndex < Sound.ChannelCount; ++ChannelIndex)
                {
                    fwrite(Sound.Samples[ChannelIndex], Destination->Sound.SampleCount*sizeof(i16), 1, Out);
                }

                free(Sound.Free);
            }
            else
            {
                loaded_bitmap Bitmap;
                if (Source->Type == AssetType_Font)
                {
                    Bitmap = LoadGlyphBitmap(Source->FileName, Source->Codepoint);
                } 
                else
                {
                    Bitmap = LoadBitmap(Source->FileName);
                }

                Destination->Bitmap.Dimension[0] = Bitmap.Width;
                Destination->Bitmap.Dimension[1] = Bitmap.Height;

                OM_ASSERT((Bitmap.Width*4) == Bitmap.Pitch);
                fwrite(Bitmap.Memory, Bitmap.Width * Bitmap.Height * 4, 1, Out);

                free(Bitmap.Free);
            }
        }
        fseek(Out, (u32)Header.AssetsOffset, SEEK_SET);
        fwrite(Assets->Assets, AssetArraySize, 1, Out);

        fclose(Out);
    }
    else
    {
        printf("ERROR: Couldn't open file.");
    }
}

om_internal void
Initialize(game_assets *Assets)
{
    Assets->TagCount = 1;
    Assets->AssetCount = 1;
    Assets->DEBUGAssetType = 0;
    Assets->AssetIndex = 0;

    Assets->AssetTypeCount = Asset_Type_Count;
    memset(Assets->AssetTypes, 0, sizeof(Assets->AssetTypes));
}

om_internal void
WritePlayer(void)
{
    game_assets Assets_;
    game_assets *Assets = &Assets_;
    Initialize(Assets);

    BeginAssetType(Assets, Asset_Type_Player);
    AddBitmapAsset(Assets, "C:\\Users\\Oskar\\Documents\\GitHub\\game\\Data\\playerBitmap.bmp");
    EndAssetType(Assets);

    WriteGA(Assets, "test1.ga");
}

om_internal void
WriteNonPlayer(void)
{
    game_assets Assets_;
    game_assets *Assets = &Assets_;
    Initialize(Assets);

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

    BeginAssetType(Assets, Asset_Type_Font);
    for (u32 Character = 'A'; Character <= 'Z'; ++Character)
    {
        AddCharacterAsset(Assets, "C:/Windows/Fonts/arial.ttf", Character);
        AddAssetTag(Assets, Asset_Tag_UnicodeCodepoint, (r32)Character);
    }
    EndAssetType(Assets);

    WriteGA(Assets, "test2.ga");
}

om_internal void
WriteSounds(void)
{
    game_assets Assets_;
    game_assets *Assets = &Assets_;
    Initialize(Assets);

    //TODO: This is temporary code for testing. Should be removed once we load this in from asset files.
    u32 SingleMusicChunk = 10 * 48000;
    u32 TotalMusicSampleCount = 7468095;

    BeginAssetType(Assets, Asset_Type_Music);
    for (u32 FirstSampleIndex = 0; FirstSampleIndex < TotalMusicSampleCount; FirstSampleIndex += SingleMusicChunk)
    {
        u32 SampleCount = TotalMusicSampleCount - FirstSampleIndex;
        if (SampleCount > SingleMusicChunk)
        {
            SampleCount = SingleMusicChunk;
        }
        sound_id ThisMusic = AddSoundAsset(Assets, "C:\\Users\\Oskar\\Documents\\GitHub\\game\\Data\\music_test.wav", FirstSampleIndex, SampleCount);
        if ((FirstSampleIndex + SingleMusicChunk) < TotalMusicSampleCount)
        {
            Assets->Assets[ThisMusic.Value].Sound.Chain = GASoundChain_Advance;
        }
    }
    EndAssetType(Assets);

	WriteGA(Assets, "test3.ga");
}

int
main (int ArgCount, char **Args)
{
    WritePlayer();
    WriteNonPlayer();
    WriteSounds();

    printf("Finished.");
}