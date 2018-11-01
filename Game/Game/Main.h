struct sdl_offscreen_buffer
{
	SDL_Texture *Texture;
	void *Memory;
	int Width;
	int Height;
	int Pitch;
};

struct sdl_window_dimension
{
	int Width;
	int Height;
};

struct sdl_sound_output
{
	int SamplesPerSecond;
	u32 RunningSampleIndex;
	int BytesPerSample;
	int SecondaryBufferSize;
	r32 tSine;
	int LatencySampleCount;
};
