#ifndef __PUCK_GAME_COMMON_H_
#define __PUCK_GAME_COMMON_H_

#include "jr_memmanager.h"
#include "jr_renderer.h"
#include "jr_bitmap.h"
#include "jr_sound.h"

/*
	Shared definitions by both the Platform and Game layers.
*/
enum button_state
{
	None, Released, Pressed, Held
};

button_state UpdateButton(bool current, button_state prev)
{
	if (current)
	{
		if (prev == Pressed || prev == Held)
			return Held;
		else
			return Pressed;
	}
	else if (prev == Pressed || prev == Held)
		return Released;
	else
		return None;
}

struct controllerState
{
	int16 lStickX;
	int16 lStickY;
	int16 rStickX;
	int16 rStickY;
	button_state up;
	button_state down;
	button_state left;
	button_state right;
	button_state start;
	button_state back;
	button_state lThumb;
	button_state rThumb;
	button_state lShoulder;
	button_state rShoulder;
	button_state aButton;
	button_state bButton;
	button_state xButton;
	button_state yButton;

	controllerState() 
		: up(None), down(None), left(None), right(None), 
		  start(None), back(None), lThumb(None), rThumb(None), 
		  lShoulder(None), rShoulder(None), aButton(None), 
		  bButton(None), xButton(None), yButton(None)
	{}
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