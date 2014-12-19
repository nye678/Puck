#include "puck.h"
#include "math.h"
#include "jr_color.h"

using namespace jr;
using namespace puck;

void DrawRectangle(game_renderer* renderer, const rect &r)
{
	if (!renderer->buffer)
		return;

	int left = r.left > 0.0f ? r.left : 0.0f;
	int right = r.right < 1024.0f ? r.right : 1023.0f;
	int top = r.top > 0.0f ? r.top : 0.0f;
	int bottom = r.bottom < 768.0f ? r.bottom : 767.0f;

	uint32_t* buffer = (uint32_t*)renderer->buffer;
	for (size_t y = top; y <= bottom; ++y)
		for (size_t x = left; x <= right; ++x)
		{
			buffer[x + y * 1024] = color::Green;
		}
}

void puck::GameUpdate(game_data* game)
{
	auto input = game->input;
	auto soundplayer = game->soundplayer;
	auto state = game->state;

	DrawRectangle(game->renderer, rect(state->puckPos.x, state->puckPos.x + 20.0f, state->puckPos.y, state->puckPos.y + 20.0f));
	DrawRectangle(game->renderer, rect(state->p1x, state->p1x + 20.0f, state->p1y, state->p1y + 100.0f));
	DrawRectangle(game->renderer, rect(state->p2x, state->p2x + 20.0f, state->p2y, state->p2y + 100.0f));

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
		soundplayer->test = true;
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
		soundplayer->test = true;
	}

	if (state->puckPos.x < 0.0f)
	{
		state->puckPos.x = windowWidth / 2.0f;
		state->puckPos.y = windowHeight / 2.0f;
	}
	else if (state->puckPos.x > windowWidth - 20.0f)
	{
		state->puckPos.x = windowWidth / 2.0f;
		state->puckPos.y = windowHeight / 2.0f;
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