#ifndef __PUCK_H_
#define __PUCK_H_

#include "jr_vec.h"
#include "jr_matrix.h"
#include "jr_shapes.h"
#include "jr_memmanager.h"
#include "jr_renderbuffer.h"
#include "jr_bitmap.h"
#include "stdint.h"

namespace puck
{

static int windowWidth = 1024;
static int windowHeight = 768;

static jr::vec3 squareVerts[4] =
{
	jr::vec3(0.0f, 0.0f, 0.0f),
	jr::vec3(1.0f, 0.0f, 0.0f),
	jr::vec3(1.0f, -1.0f, 0.0f),
	jr::vec3(0.0f, -1.0f, 0.0f)
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
	uint8_t* buffer;
	uint32_t bufferWidth;
	uint32_t bufferHeight;
};

struct game_soundplayer
{
	bool test;
};

void PlaySound(int soundID);

struct game_state
{
	jr::BitMap* fontBitMap;
	jr::BitMap* titleBitMap;
	jr::vec3 puckPos, puckVelocity;
	float puckSpeed, puckMaxSpeed;
	float p1x, p1y, p1speed;
	float p2x, p2y, p2speed;
	char p1Score, p2Score;
};

void InitializeGameState(game_state* state)
{
	state->p1x = 5.0f;
	state->p1y = windowHeight/2.0f - 50.0f;
	state->p1speed = 10.0f;

	state->p2x = windowWidth - 25.0f;
	state->p2y = windowHeight/2.0f - 50.0f;
	state->p2speed = 10.0f;

	state->puckPos = jr::vec3(windowWidth / 2.0f, windowHeight / 2.0f, 0.0f);
	state->puckVelocity = jr::vec3(2.0f, 2.0f, 0.0f);
	state->puckSpeed = 2.0f;
	state->puckMaxSpeed = 10.0f;

	state->p1Score = '0';
	state->p2Score = '0';
}

struct game_data
{
	jr::MemManager* mem;
	jr::RenderBuffer* renderer;
	game_soundplayer* soundplayer;
	game_input* input;
	game_state* state;
};

void GameUpdate(game_data* game);

}

#endif