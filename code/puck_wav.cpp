#include "puck_wav.h"
#include <Windows.h>
#include <assert.h>

jr::Sound* ReadWave(jr::MemManager* mem, const char* filePath)
{
	WaveChunkDescriptor desc;
	WaveFormatChunk __unaligned format;
	WaveDataChunk data;

	HANDLE wavHandle = CreateFile(
		filePath,
		GENERIC_READ,
		FILE_SHARE_READ,
		nullptr,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		nullptr);

	if (wavHandle == INVALID_HANDLE_VALUE)
		return nullptr;

	DWORD bytesRead = 0;
	if (!ReadFile(wavHandle, (void*)(&desc), sizeof(WaveChunkDescriptor), &bytesRead, nullptr))
	{
		CloseHandle(wavHandle);
		return nullptr;
	}

	if (desc.chunkId != PUCK_RIFF_ID || desc.format != PUCK_WAVE_ID)
	{
		CloseHandle(wavHandle);
		return nullptr;
	}

	if (!ReadFile(wavHandle, (void*)(&format), sizeof(WaveFormatChunk), &bytesRead, nullptr))
	{
		CloseHandle(wavHandle);
		return nullptr;
	}

	if (!ReadFile(wavHandle, (void*)(&data), sizeof(WaveDataChunk), &bytesRead, nullptr))
	{
		CloseHandle(wavHandle);
		return nullptr;
	}

	uintptr_t soundmemory = (uintptr_t)mem->Alloc(sizeof(jr::Sound) + data.dataChunkSize);
	jr::Sound* sound = (jr::Sound*)soundmemory;
	sound->audioBytes = data.dataChunkSize;
	sound->numChannels = format.numChannels;
	sound->sampleRate = format.sampleRate;
	sound->buffer = (uint8_t*)(soundmemory + sizeof(jr::Sound));

	ReadFile(wavHandle, (void*)(sound->buffer), data.dataChunkSize, &bytesRead, nullptr);
	CloseHandle(wavHandle);
	return sound;
}
