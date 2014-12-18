#ifndef __PUCK_H_
#define __PUCK_H_

#include "puck_math.h"
#include "puck_mem.h"
#include "stdint.h"

namespace puck
{

static int windowWidth = 1024;
static int windowHeight = 768;

static vec3 squareVerts[4] =
{
	vec3(0.0f, 0.0f, 0.0f),
	vec3(1.0f, 0.0f, 0.0f),
	vec3(1.0f, -1.0f, 0.0f),
	vec3(0.0f, -1.0f, 0.0f)
};

static short squareIndicies[6] =
{
	0, 1, 2, 3, 0, 2
};

static int squareIndexOffset = 0;

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

struct game_renderer
{
	unsigned short indices[10000];
	int numIndicies;
	mat4 transforms[1000];
	int numTransforms;
};

void DrawRectangle(game_renderer* renderer, const rect &r);

struct game_soundplayer
{
	bool test;
};

void PlaySound(int soundID);

struct game_state
{
	vec3 puckPos, puckVelocity;
	float puckSpeed, puckMaxSpeed;
	float p1x, p1y, p1speed;
	float p2x, p2y, p2speed;
};

void InitializeGameState(game_state* state)
{
	state->p1x = 5.0f;
	state->p1y = windowHeight/2.0f - 50.0f;
	state->p1speed = 10.0f;

	state->p2x = windowWidth - 25.0f;
	state->p2y = windowHeight/2.0f - 50.0f;
	state->p2speed = 10.0f;

	state->puckPos = vec3(windowWidth / 2.0f, windowHeight / 2.0f, 0.0f);
	state->puckVelocity = vec3(2.0f, 2.0f, 0.0f);
	state->puckSpeed = 2.0f;
	state->puckMaxSpeed = 10.0f;
}

struct game_data
{
	MemManager* mem;
	game_renderer* renderer;
	game_soundplayer* soundplayer;
	game_input* input;
	game_state* state;
};

void GameUpdate(game_data* game);

}

#endif