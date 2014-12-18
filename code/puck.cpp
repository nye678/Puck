#include "puck.h"
#include "math.h"

using namespace puck;

void puck::DrawRectangle(game_renderer* renderer, const rect &r)
{
	for (int i = 0; i < 6; ++i)
	{
		renderer->indices[renderer->numIndicies + i] = squareIndicies[i];
	}
	renderer->numIndicies += 6;

	mat4 transform = mat4(1.0f);
	transform.column[0].x = r.top;
	transform.column[1].y = r.bottom;

	transform.column[3].x = r.left;
	transform.column[3].y = -r.right;
	transform.column[3].z = 0.0f;

	renderer->transforms[renderer->numTransforms] = transform;
	renderer->numTransforms++;
}

void puck::GameUpdate(game_data* game)
{
	auto input = game->input;
	auto soundplayer = game->soundplayer;
	auto state = game->state;

	DrawRectangle(game->renderer, rect(state->puckPos.x, state->puckPos.y, 20.0f, 20.0f));
	DrawRectangle(game->renderer, rect(state->p1x, state->p1y, 20.0f, 100.0f));
	DrawRectangle(game->renderer, rect(state->p2x, state->p2y, 20.0f, 100.0f));

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

	float magnitude = sqrt((float)(input->controllers[0].lStickX * input->controllers[0].lStickX) + (float)(input->controllers[0].lStickY * input->controllers[0].lStickY));
	float normalizedLX = input->controllers[0].lStickX / magnitude;
	float normalizedLY = input->controllers[0].lStickY / magnitude;
	float normalizedMagnitude = 0;
	int leftDeadZone = 7849;
	if (magnitude > leftDeadZone)
	{
		if (magnitude > 32767)
			magnitude = 32767;
		magnitude -= leftDeadZone;
		normalizedMagnitude = magnitude / (32767 - leftDeadZone);

		state->p1y -= normalizedLY * state->p1speed;
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