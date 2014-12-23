#ifndef __PUCK_H_
#define __PUCK_H_

#include "jr_vec.h"
#include "jr_matrix.h"
#include "jr_shapes.h"
#include "jr_memmanager.h"
#include "jr_renderer.h"
#include "jr_bitmap.h"
#include "stdint.h"

static int windowWidth = 1024;
static int windowHeight = 768;

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

struct game_soundplayer
{
	bool paddleSound;
	bool scoreSound;
};

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

struct game_data
{
	jr::MemManager* mem;
	jr::Renderer* renderer;
	game_soundplayer* soundplayer;
	game_input* input;
	game_state* state;
};

void InitializeGameState(game_state* state);
void GameUpdate(game_data* game);

#endif