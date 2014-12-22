#include "puck.h"
#include "math.h"
#include "jr_color.h"
#include "jr_draw.h"

using namespace jr;

rect CharToRect(char c)
{
	int tileWidth = 54, tileHeight = 76;
	int xindex = (c - 32) % 16;
	int yindex = (int)((float)(c - 32) / 16.0f);
	return rect(
		xindex * tileWidth, (xindex + 1) * tileWidth,
		yindex * tileHeight, (yindex + 1) * tileHeight);
}

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
	state->puckMaxSpeed = 15.0f;

	state->p1Score = '0';
	state->p2Score = '0';
}

void GameUpdate(game_data* game)
{
	auto input = game->input;
	auto soundplayer = game->soundplayer;
	auto state = game->state;

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

	if (game->renderer->buffer[0])
	{
		DrawBitMap(game->renderer, 0, game->state->titleBitMap, 0, 0);
		DrawBitMapTile(game->renderer, 0, game->state->fontBitMap, 500, 0, colon);
		DrawBitMapTile(game->renderer, 0, game->state->fontBitMap, 500-50, 0, p1Score);
		DrawBitMapTile(game->renderer, 0, game->state->fontBitMap, 500+50, 0, p2Score);
		DrawRectangle(game->renderer, 0, puck, 0x00FF0088);
		DrawRectangle(game->renderer, 0, paddle1, 0x0000FF88);
		DrawRectangle(game->renderer, 0, paddle2, 0xFF000088);
		DrawRectangleLine(game->renderer, 0, puck, color::Green);
		DrawRectangleLine(game->renderer, 0, paddle1, color::Blue);
		DrawRectangleLine(game->renderer, 0, paddle2, color::Red);
	}

	if (input->controllers[0].up)
	{
		state->p1y -= state->p1speed;
	}

	if (input->controllers[0].down)
	{
		state->p1y += state->p1speed;
	}

	if (input->controllers[0].aButton)
	{

	}

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
		if (state->p1y > windowHeight - 105.0f)
			state->p1y = windowHeight - 105.0f;
	}

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

		state->p2x = windowWidth - 25.0f;
		state->p2y -= normalizedRY * state->p2speed;
		if (state->p2y < 5.0f)
			state->p2y = 5.0f;
		if (state->p2y > windowHeight - 105.0f)
			state->p2y = windowHeight - 105.0f;
	}

	state->puckPos.x += state->puckVelocity.x;
	state->puckPos.y += state->puckVelocity.y;

	if (state->puckPos.x < state->p1x + 20.0f 
		&& state->puckPos.y < state->p1y + 100.0f
		&& state->puckPos.y + 20.0f > state->p1y)
	{
		state->puckPos.x = state->p1x + 20.5f;
		if (state->puckSpeed < state->puckMaxSpeed)
			state->puckSpeed += 0.5f;
		state->puckVelocity = normalize(state->puckVelocity) * state->puckSpeed;
		state->puckVelocity.x *= -1.0f;
		soundplayer->paddleSound = true;
	}
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
		soundplayer->paddleSound = true;
	}

	if (state->puckPos.x < 0.0f)
	{
		state->puckPos.x = windowWidth / 2.0f;
		state->puckPos.y = windowHeight / 2.0f;
		state->puckVelocity *= 0.7f;
		state->p2Score++;
		soundplayer->scoreSound = true;
	}
	else if (state->puckPos.x > windowWidth - 20.0f)
	{
		state->puckPos.x = windowWidth / 2.0f;
		state->puckPos.y = windowHeight / 2.0f;
		state->puckVelocity *= 0.7f;
		state->p1Score++;
		soundplayer->scoreSound = true;
	}

	if (state->p1Score == ':' || state->p2Score == ':')
	{
		state->p1Score = '0';
		state->p2Score = '0';
	}

	if (state->puckPos.y < 0.0f)
	{
		state->puckPos.y = 0.0f;
		state->puckVelocity.y *= -1.0f;
	}
	else if (state->puckPos.y > windowHeight - 20.0f)
	{
		state->puckPos.y = windowHeight - 20.0f;
		state->puckVelocity.y *= -1.0f;
	}
}