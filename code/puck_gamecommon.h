#ifndef __PUCK_GAME_COMMON_H_
#define __PUCK_GAME_COMMON_H_

#include "jr_memmanager.h"
#include "jr_renderer.h"
#include "jr_bitmap.h"
#include "jr_sound.h"
#include <stdint.h>

struct controllerState
{
	int16_t lStickX;
	int16_t lStickY;
	int16_t rStickX;
	int16_t rStickY;
	bool up;
	bool down;
	bool left;
	bool right;
	bool start;
	bool back;
	bool lThumb;
	bool rThumb;
	bool lShoulder;
	bool rShoulder;
	bool aButton;
	bool bButton;
	bool xButton;
	bool yButton;
};

struct game_input
{
	controllerState controllers[4];
};

// Total hack. Needs to be replaced. Can only play
// a single sound at a time.
struct game_soundplayer
{
	jr::Sound* sound;
};

typedef jr::BitMap* (*DebugLoadBitMapFunc)(const char*);
typedef jr::Sound* (*DebugLoadSoundFunc)(const char*);

struct debug_tools
{
	DebugLoadBitMapFunc LoadBitMap;
	DebugLoadSoundFunc LoadSound;
};

struct Systems
{
	jr::MemManager* mem;
	jr::Renderer* renderer;
	game_soundplayer* soundplayer;
	game_input* input;
	debug_tools* debug;
};

#endif