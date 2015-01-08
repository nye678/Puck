#include "puck_bmp.h"
#include "jr_color.h"
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

	size_t imageBufferSize = imageHeader.imageWidth * imageHeader.imageHeight * sizeof(uint32);
	uintptr bitMapMemory = (uintptr)mem->Alloc(sizeof(jr::BitMap) + imageBufferSize);

	jr::BitMap* bitMap = (jr::BitMap*)bitMapMemory;
	bitMap->data = (uint32*)(bitMapMemory + sizeof(jr::BitMap));
	bitMap->width = imageHeader.imageWidth;
	bitMap->height = imageHeader.imageHeight;

	jr::ScopeStack stack = mem->PushScope();
	uint32* pixels = (uint32*)stack.Alloc(sizeof(uint32) * imageHeader.imageWidth);
	for (int i = imageHeader.imageHeight - 1; i >= 0 ; --i)
	{
		ReadFile(bmpHandle, (void*)pixels, sizeof(uint32) * imageHeader.imageWidth, &bytesRead, nullptr);

		for (int x = 0; x < imageHeader.imageWidth; ++x)
		{
			bitMap->data[x + i * imageHeader.imageWidth] = pixels[x];
		}
	}

	CloseHandle(bmpHandle);
	return bitMap;
}