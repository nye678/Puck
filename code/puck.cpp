#include "puck_gamecommon.h"
#include "math.h"
#include "jr_color.h"
#include "jr_draw.h"
#include "jr_bitmap.h"
#include "jr_vec.h"

using namespace jr;

/*
	Puck's one and only game state.
*/
struct game_state
{
	jr::BitMap* fontBitMap;
	jr::BitMap* titleBitMap;
	jr::Sound* paddleSfx;
	jr::Sound* scoreSfx;
	jr::vec3 puckPos, puckVelocity;
	float puckSpeed, puckMaxSpeed;
	float p1x, p1y, p1speed;
	float p2x, p2y, p2speed;
	char p1Score, p2Score;
};

/*
	Global pointer to the state.
*/
game_state* state;

/*
	Initializes the game state.
	NOTE: Once resource loading is in then load up the few resources we need here.
*/
extern "C" __declspec(dllexport) void InitializeGame(Systems* sys)
{
	float width = (float)sys->renderer->bufferWidth;
	float height = (float)sys->renderer->bufferHeight;

	state = (game_state*)sys->mem->Alloc(sizeof(game_state));

	state->fontBitMap = sys->debug->LoadBitMap("font.bmp");
	state->titleBitMap = sys->debug->LoadBitMap("superpuck.bmp");
	state->scoreSfx = sys->debug->LoadSound("Pickup_Coin.wav");
	state->paddleSfx = sys->debug->LoadSound("blip.wav");

	state->p1x = 5.0f;
	state->p1y = height/2.0f - 50.0f;
	state->p1speed = 10.0f;

	state->p2x = width - 25.0f;
	state->p2y = height/2.0f - 50.0f;
	state->p2speed = 10.0f;

	state->puckPos = vec3(width / 2.0f, height / 2.0f, 0.0f);
	state->puckVelocity = vec3(2.0f, 2.0f, 0.0f);
	state->puckSpeed = 2.0f;
	state->puckMaxSpeed = 15.0f;

	state->p1Score = '0';
	state->p2Score = '0';
}

/*
	Converts a char into a rectangle to be used with the font bitmap
	while renderering characters.
*/
rect CharToRect(char c)
{
	int tileWidth = 54, tileHeight = 76;
	int xindex = (c - 32) % 16;
	int yindex = (int)((float)(c - 32) / 16.0f);
	return rect(
		xindex * tileWidth, (xindex + 1) * tileWidth,
		yindex * tileHeight, (yindex + 1) * tileHeight);
}

/*
	Updates the game state each frame.
	NOTE: This code is super hacked in at the moment.
*/
extern "C" __declspec(dllexport) void GameUpdate(Systems* sys)
{
	float width = (float)sys->renderer->bufferWidth;
	float height = (float)sys->renderer->bufferHeight;

	auto input = sys->input;
	auto soundplayer = sys->soundplayer;

	/*
		D-pad input
		NOTE: Only works on player 1 paddle.
	*/
	if (input->controllers[0].up)
	{
		state->p1y -= state->p1speed;
	}

	if (input->controllers[0].down)
	{
		state->p1y += state->p1speed;
	}

	/*
		Analog stick controls for player 1.
	*/
	controllerState &controller = input->controllers[0];
	vec2 leftStick = vec2((float)controller.lStickX, (float)controller.lStickY);
	vec2 leftNormalized = normalize(leftStick);
	float magnitude = length(leftStick);
	float normalizedMagnitude = 0;
	int leftDeadZone = 7849;
	if (magnitude > leftDeadZone)
	{
		if (magnitude > 32767)
			magnitude = 32767;
		magnitude -= leftDeadZone;
		normalizedMagnitude = magnitude / (32767 - leftDeadZone);

		state->p1y -= leftNormalized.y * state->p1speed;
		if (state->p1y < 5.0f)
			state->p1y = 5.0f;
		if (state->p1y > height - 105.0f)
			state->p1y = height - 105.0f;
	}

	/*
		Analog stick controls for player 2.
		NOTE: This on the right stick of the p1 controller for debug. Need to switch over to player 2 controller instead.
	*/
	float magnitude2 = sqrt((float)(input->controllers[0].rStickX * input->controllers[0].rStickX) + (float)(input->controllers[0].rStickY * input->controllers[0].rStickY));
	float normalizedRX = input->controllers[0].rStickX / magnitude2;
	float normalizedRY = input->controllers[0].rStickY / magnitude2;
	float normalizedMagnitude2 = 0;
	int rightDeadZone = 8689;
	if (magnitude2 > rightDeadZone)
	{
		if (magnitude2 > 32767)
			magnitude2 = 32767;
		magnitude2 -= rightDeadZone;
		normalizedMagnitude2 = magnitude2 / (32767 - rightDeadZone);

		state->p2x = width - 25.0f;
		state->p2y -= normalizedRY * state->p2speed;
		if (state->p2y < 5.0f)
			state->p2y = 5.0f;
		if (state->p2y > height - 105.0f)
			state->p2y = height - 105.0f;
	}

	/*
		Update the puck state.
	*/
	state->puckPos.x += state->puckVelocity.x;
	state->puckPos.y += state->puckVelocity.y;

	/*
		Check to see if the puck hit the player 1 paddle.
	*/
	if (state->puckPos.x < state->p1x + 20.0f 
		&& state->puckPos.y < state->p1y + 100.0f
		&& state->puckPos.y + 20.0f > state->p1y)
	{
		state->puckPos.x = state->p1x + 20.5f;
		if (state->puckSpeed < state->puckMaxSpeed)
			state->puckSpeed += 0.5f;
		state->puckVelocity = normalize(state->puckVelocity) * state->puckSpeed;
		state->puckVelocity.x *= -1.0f;
		soundplayer->sound = state->paddleSfx;
	}

	/*
		Check to see if the puck hit the player 2 paddle.
	*/
	if (state->puckPos.x + 20.0f > state->p2x
		&& state->puckPos.y < state->p2y + 100.0f
		&& state->puckPos.y + 20.0f > state->p2y
		)
	{
		state->puckPos.x = state->p2x - 20.5f;
		if (state->puckSpeed < state->puckMaxSpeed)
			state->puckSpeed += 0.5f;
		state->puckVelocity = normalize(state->puckVelocity) * state->puckSpeed;
		state->puckVelocity.x *= -1.0f;
		soundplayer->sound = state->paddleSfx;
	}

	/*
		Check to see if the puck scored for either player 1 or player 2.
	*/
	if (state->puckPos.x < 0.0f)
	{
		state->puckPos.x = width / 2.0f;
		state->puckPos.y = height / 2.0f;
		state->puckVelocity *= 0.7f;
		state->p2Score++;
		soundplayer->sound = state->scoreSfx;
	}
	else if (state->puckPos.x > width - 20.0f)
	{
		state->puckPos.x = width / 2.0f;
		state->puckPos.y = height / 2.0f;
		state->puckVelocity *= 0.7f;
		state->p1Score++;
		soundplayer->sound = state->scoreSfx;
	}

	/*
		Resets the score once the a player scores 10 times.
		NOTE: the ':' character is the ascii character after the 9 character.
	*/
	if (state->p1Score == ':' || state->p2Score == ':')
	{
		state->p1Score = '0';
		state->p2Score = '0';
	}

	/*
		Check to see if the puck hits either the top or bottom of the play area.
	*/
	if (state->puckPos.y < 0.0f)
	{
		state->puckPos.y = 0.0f;
		state->puckVelocity.y *= -1.0f;
	}
	else if (state->puckPos.y > height - 20.0f)
	{
		state->puckPos.y = height - 20.0f;
		state->puckVelocity.y *= -1.0f;
	}

	/*
		Creates the rectangles used to draw the puck, paddles, and characters.
	*/
	rect puck = rect(
		state->puckPos.x, state->puckPos.x + 20.0f, 
		state->puckPos.y, state->puckPos.y + 20.0f);
	rect paddle1 = rect(
		state->p1x, state->p1x + 20.0f,
		state->p1y, state->p1y + 100.0f);
	rect paddle2 = rect(
		state->p2x, state->p2x + 20.0f,
		state->p2y, state->p2y + 100.0f);

	rect colon = CharToRect(':');
	rect p1Score = CharToRect(state->p1Score);
	rect p2Score = CharToRect(state->p2Score);

	/*
		Render the current game state.
	*/
	if (sys->renderer)
	{
		DrawBitMap(sys->renderer, 0, state->titleBitMap, 0, 0);
		DrawBitMapTile(sys->renderer, 0, state->fontBitMap, 500, 0, colon);
		DrawBitMapTile(sys->renderer, 0, state->fontBitMap, 500-50, 0, p1Score);
		DrawBitMapTile(sys->renderer, 0, state->fontBitMap, 500+50, 0, p2Score);
		DrawRectangle(sys->renderer, 0, puck, 0x00FF0088);
		DrawRectangle(sys->renderer, 0, paddle1, 0x0000FF88);
		DrawRectangle(sys->renderer, 0, paddle2, 0xFF000088);
		DrawRectangleLine(sys->renderer, 0, puck, color::Green);
		DrawRectangleLine(sys->renderer, 0, paddle1, color::Blue);
		DrawRectangleLine(sys->renderer, 0, paddle2, color::Red);
	}
}