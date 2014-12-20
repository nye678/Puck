#ifndef __PUCK_BMP_H_
#define __PUCK_BMP_H_

#include "jr_memmanager.h"
#include "jr_bitmap.h"

#pragma pack(1)
struct BitMapFileHeader
{
	char type1;
	char type2;
	int fileSize;
	short res1;
	short res2;
	int pixelOffset;
};

struct BitMapImageHeader
{
	int headerSize;
	int imageWidth;
	int imageHeight;
	short planes;
	short bitCount;
	int compression;
	int imageSize;
	int xPixelsPerMeter;
	int yPixelsPerMeter;
	int usedColorMapEntries;
	int numSignificantColors;
};
#pragma pack()

jr::BitMap* ReadBMP(jr::MemManager* mem, const char* filePath);

#endif