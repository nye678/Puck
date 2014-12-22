#include "puck_bmp.h"
#include <Windows.h>
#include <assert.h>

jr::BitMap* ReadBMP(jr::MemManager* mem, const char* filePath)
{
	BitMapFileHeader __unaligned fileHeader;
	BitMapImageHeader imageHeader;

	HANDLE bmpHandle = CreateFile(
		filePath,
		GENERIC_READ,
		FILE_SHARE_READ,
		nullptr,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		nullptr);

	if (bmpHandle == INVALID_HANDLE_VALUE)
		return nullptr;

	DWORD bytesRead = 0;
	if (!ReadFile(bmpHandle, (void*)(&fileHeader), sizeof(BitMapFileHeader), &bytesRead, nullptr))
	{
		CloseHandle(bmpHandle);
		return nullptr;
	}

	if (!ReadFile(bmpHandle, (void*)(&imageHeader), sizeof(BitMapImageHeader), &bytesRead, nullptr))
	{
		CloseHandle(bmpHandle);
		return nullptr;
	}

	LARGE_INTEGER pixelOffset;
	pixelOffset.QuadPart = fileHeader.pixelOffset;
	if (!SetFilePointerEx(bmpHandle, pixelOffset, nullptr, FILE_BEGIN))
	{
		CloseHandle(bmpHandle);
		return nullptr;
	}

	size_t imageBufferSize = imageHeader.imageWidth * imageHeader.imageHeight * sizeof(uint32_t);
	uintptr_t bitMapMemory = (uintptr_t)mem->Alloc(sizeof(jr::BitMap) + imageBufferSize);

	jr::BitMap* bitMap = (jr::BitMap*)bitMapMemory;
	bitMap->bitMap = (uint32_t*)(bitMapMemory + sizeof(jr::BitMap));
	bitMap->width = imageHeader.imageWidth;
	bitMap->height = imageHeader.imageHeight;

	jr::ScopeStack stack = mem->PushScope();
	uint8_t* pixels = (uint8_t*)stack.Alloc(4 * imageHeader.imageWidth);
	for (int i = imageHeader.imageHeight - 1; i >= 0 ; --i)
	{
		ReadFile(bmpHandle, (void*)pixels, 4 * imageHeader.imageWidth, &bytesRead, nullptr);

		for (int x = 0; x < imageHeader.imageWidth; ++x)
		{
			bitMap->bitMap[x + i * imageHeader.imageWidth] = 
				(uint32_t)pixels[4 * x] << 24 |
				(uint32_t)pixels[4 * x + 1] << 16 |
				(uint32_t)pixels[4 * x + 2] << 8 |
				(uint32_t)pixels[4 * x + 3];
		}
	}

	CloseHandle(bmpHandle);
	return bitMap;
}