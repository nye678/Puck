#ifndef __PUCK_WAVE_H_
#define __PUCK_WAVE_H_

#include "jr_memmanager.h"
#include "jr_sound.h"

#define PUCK_RIFF_ID 0x46464952
#define PUCK_WAVE_ID 0x45564157
#define PUCK_FTM_ID 0x20746566
#define PUCK_AUDIO_FORMAT_PCM 1

struct WaveChunkDescriptor
{
	uint32 chunkId;
	uint32 chunkSize;
	uint32 format;
};

#pragma pack(1)
struct WaveFormatChunk
{
	uint32 formatChunkId;
	uint32 formatChunkSize;
	uint16 audioFormat;
	uint16 numChannels;
	uint32 sampleRate;
	uint32 byteRate;
	uint16 blockAlign;
	uint16 bitsPerSample;
};
#pragma pack()

struct WaveDataChunk
{
	uint32 dataChunkId;
	uint32 dataChunkSize;
};

jr::Sound* ReadWave(jr::MemManager* mem, const char* filePath);

#endif