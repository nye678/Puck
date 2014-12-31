#ifndef __PUCK_GAME_COMMON_H_
#define __PUCK_GAME_COMMON_H_

#include "jr_memmanager.h"
#include "jr_renderer.h"
#include "jr_bitmap.h"
#include "jr_sound.h"

/*
	Shared definitions by both the Platform and Game layers.
*/

struct controllerState
{
	int16 lStickX;
	int16 lStickY;
	int16 rStickX;
	int16 rStickY;
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

/*
	Debug function pointers. These functions will be available
	to the game to call down to platform code for debugging
	purposes. Not to be used in final ship code.
*/
typedef jr::BitMap* (*DebugLoadBitMapFunc)(const char*);
typedef jr::Sound* (*DebugLoadSoundFunc)(const char*);

struct debug_tools
{
	DebugLoadBitMapFunc LoadBitMap;
	DebugLoadSoundFunc LoadSound;
};

/*
	Foward facing interface passed by the platform to the game.
	References to these objects should not be held across multiple
	frames as some of them may point to different memory locations
	each frame.
*/
struct Systems
{
	jr::MemManager* mem;
	jr::Renderer* renderer;
	game_soundplayer* soundplayer;
	game_input* input;
	debug_tools* debug;
	void* state;
};

#endif