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
	uint32_t chunkId;
	uint32_t chunkSize;
	uint32_t format;
};

#pragma pack(1)
struct WaveFormatChunk
{
	uint32_t formatChunkId;
	uint32_t formatChunkSize;
	uint16_t audioFormat;
	uint16_t numChannels;
	uint32_t sampleRate;
	uint32_t byteRate;
	uint16_t blockAlign;
	uint16_t bitsPerSample;
};
#pragma pack()

struct WaveDataChunk
{
	uint32_t dataChunkId;
	uint32_t dataChunkSize;
};

jr::Sound* ReadWave(jr::MemManager* mem, const char* filePath);

#endif