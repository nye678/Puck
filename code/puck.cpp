#include "puck_gamecommon.h"
#include "jr_color.h"
#include "jr_draw.h"
#include "jr_bitmap.h"
#include "jr_vec.h"
#include "jr_matrix.h"

using namespace jr;

struct paddle_data
{
	Color color;
	jr::vec2 pos;
	float angle;
	float halfWidth, halfHeight;
	float speed;
	char score;
};

void DrawPaddle(jr::Renderer* renderer, paddle_data* paddle)
{
	jr::mat3 model = jr::translation(paddle->pos.x, paddle->pos.y) * jr::rotation(paddle->angle);
	jr::vec2 tl = model * jr::vec2(-paddle->halfWidth, -paddle->halfHeight);
	jr::vec2 bl = model * jr::vec2(-paddle->halfWidth, paddle->halfHeight);
	jr::vec2 tr = model * jr::vec2(paddle->halfWidth, -paddle->halfHeight);
	jr::vec2 br = model * jr::vec2(paddle->halfWidth, paddle->halfHeight);

	DrawTriangle(renderer, 0, tl, tr, bl, paddle->color);
	DrawTriangle(renderer, 0, tr, br, bl, paddle->color);

	rect r(paddle->pos.x - paddle->halfWidth, paddle->pos.x + paddle->halfWidth, paddle->pos.y - paddle->halfHeight, paddle->pos.y + paddle->halfHeight);
	//DrawRectangle(renderer, 0, r, paddle->color);
}

void UpdatePaddle(paddle_data* paddle, float minY, float maxY, int16 stickX, int16 stickY, int32 deadzone)
{
	vec2 stick = vec2((float)stickX, (float)stickY);
	vec2 normalizedStick = normalize(stick);
	float magnitude = stick.length();
	
	if (magnitude > deadzone)
	{
		paddle->pos.y -= normalizedStick.y * paddle->speed;
		if (paddle->pos.y < minY + paddle->halfHeight)
			paddle->pos.y = minY + paddle->halfHeight;
		if (paddle->pos.y > maxY - paddle->halfHeight)
			paddle->pos.y = maxY - paddle->halfHeight;
	}
}

struct puck_data
{
	Color color;
	jr::vec2 pos;
	jr::vec2 vel;
	float halfWidth, halfHeight;
	float speed, maxSpeed;
};

void DrawPuck(jr::Renderer* renderer, puck_data* puck)
{
	rect puckRect = rect(
		puck->pos.x - puck->halfWidth, puck->pos.x + puck->halfWidth, 
		puck->pos.y - puck->halfHeight, puck->pos.y + puck->halfHeight);
	DrawRectangle(renderer, 0, puckRect, puck->color);
}

/*
	Puck's one and only game state.
*/
struct game_state
{
	puck_data puck;
	paddle_data paddle1, paddle2;

	jr::BitMap* fontBitMap;
	jr::BitMap* titleBitMap;
	jr::Sound* paddleSfx;
	jr::Sound* scoreSfx;

	bool mainmenu;
	bool paused;
};

void ResetGameState(game_state* state, float width, float height)
{
	state->paddle1.color = jr::color::Blue;
	state->paddle1.pos = vec2(15.0f, height/2.0f - 50.0f);
	state->paddle1.angle = 0.0f;
	state->paddle1.halfWidth = 10.0f;
	state->paddle1.halfHeight = 50.0f;
	state->paddle1.speed = 10.0f;
	state->paddle1.score = '0';

	state->paddle2.color = jr::color::Red;
	state->paddle2.pos = vec2(width - 15.0f, height/2.0f - 50.0f);
	state->paddle2.angle = 0.0f;
	state->paddle2.halfWidth = 10.0f;
	state->paddle2.halfHeight = 50.0f;
	state->paddle2.speed = 10.0f;
	state->paddle2.score = '0';

	state->puck.color = jr::color::Green;
	state->puck.pos = vec2(width / 2.0f, height / 2.0f);
	state->puck.vel = vec2(2.0f, 2.0f);
	state->puck.halfWidth = 10.0f;
	state->puck.halfHeight = 10.0f;
	state->puck.speed = 2.0f;
	state->puck.maxSpeed = 4.0f;

	state->mainmenu = true;
	state->paused = false;
}

/*
	Initializes the game state.
	NOTE: Once resource loading is in then load up the few resources we need here.
*/
extern "C" __declspec(dllexport) void InitializeGame(Systems* sys)
{
	float width = (float)sys->renderer->bufferWidth;
	float height = (float)sys->renderer->bufferHeight;

	game_state* state = (game_state*)sys->mem->Alloc(sizeof(game_state));

	state->titleBitMap = sys->debug->LoadBitMap("..\\data\\superpuck.bmp");
	state->fontBitMap = sys->debug->LoadBitMap("..\\data\\font.bmp");
	state->scoreSfx = sys->debug->LoadSound("..\\data\\Pickup_Coin.wav");
	state->paddleSfx = sys->debug->LoadSound("..\\data\\blip.wav");

	ResetGameState(state, width, height);
	sys->state = state;
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
	game_state* state = (game_state*)sys->state;

	float width = (float)sys->renderer->bufferWidth;
	float height = (float)sys->renderer->bufferHeight;

	auto input = sys->input;
	auto soundplayer = sys->soundplayer;

	static bool prevStart = false;
	if (prevStart && !input->controllers[0].start)
	{
		state->paused = !state->paused;
	}
	prevStart = input->controllers[0].start;

	static bool prevA = false;
	static bool startSelected = true;
	if (prevA && !input->controllers[0].aButton)
	{
		if (startSelected)
		{
			state->mainmenu = false;
		}
	}
	prevA = input->controllers[0].aButton;

	state->paddle1.angle = 0.0f;
	state->paddle2.angle = 0.0f;
	if (input->controllers[0].lShoulder)
	{
		state->paddle1.angle = JR_PI / 12.0f;
		state->paddle2.angle = JR_PI / 12.0f;
	}
	if (input->controllers[0].rShoulder)
	{
		state->paddle1.angle = JR_PI / -12.0f;
		state->paddle2.angle = JR_PI / -12.0f;
	}

	if (state->mainmenu)
	{
		static bool prevDown = false;
		if (!prevDown && input->controllers[0].down)
		{
			startSelected = !startSelected;
		}
		prevDown = input->controllers[0].down;
	
		static bool prevUp = false;
		if (!prevUp && input->controllers[0].up)
		{
			startSelected = !startSelected;
		}
		prevUp = input->controllers[0].up;
	}

	if (!state->paused && !state->mainmenu)
	{
		puck_data* puck = &state->puck;
		paddle_data* paddle1 = &state->paddle1;
		paddle_data* paddle2 = &state->paddle2;

		/*
			Analog stick controls for player 1.
		*/
		controllerState &controller = input->controllers[0];
		
		UpdatePaddle(paddle1, 5.0f, height - 5.0f, controller.lStickX, controller.lStickY, 7849);
		UpdatePaddle(paddle2, 5.0f, height - 5.0f, controller.rStickY, controller.rStickY, 8689);

		/*
			Update the puck state.
		*/
		puck->pos += puck->vel;

		/*
			Check to see if the puck hit the player 1 paddle.
		*/
		if (puck->pos.x - puck->halfWidth < paddle1->pos.x + paddle1->halfWidth
			&& puck->pos.y - puck->halfWidth < paddle1->pos.y + paddle1->halfHeight
			&& puck->pos.y + puck->halfWidth > paddle1->pos.y - paddle1->halfHeight)
		{
			puck->pos.x = paddle1->pos.x + paddle1->halfWidth + 15.0f;
			if (puck->speed < puck->maxSpeed)
				puck->speed += 0.5f;

			jr::vec2 paddleNormal = jr::rotation(paddle1->angle) * jr::vec2(1.0f, 0.0f);
			puck->vel = jr::reflect(jr::normalize(puck->vel) * puck->speed, paddleNormal);
			soundplayer->sound = state->paddleSfx;
		}

		/*
			Check to see if the puck hit the player 2 paddle.
		*/
		if (puck->pos.x + puck->halfWidth > paddle2->pos.x - paddle2->halfWidth
			&& puck->pos.y - puck->halfWidth < paddle2->pos.y + paddle2->halfHeight
			&& puck->pos.y + puck->halfWidth > paddle2->pos.y - paddle2->halfHeight)
		{
			puck->pos.x = paddle2->pos.x - paddle2->halfWidth - 15.0f;
			if (puck->speed < puck->maxSpeed)
				puck->speed += 0.5f;

			jr::vec2 paddleNormal = jr::rotation(paddle1->angle) * jr::vec2(-1.0f, 0.0f);
			puck->vel = jr::reflect(jr::normalize(puck->vel) * puck->speed, paddleNormal);
			soundplayer->sound = state->paddleSfx;
		}

		/*
			Check to see if the puck scored for either player 1 or player 2.
		*/
		if (puck->pos.x - puck->halfHeight < 0.0f)
		{
			puck->pos.x = width / 2.0f;
			puck->pos.y = height / 2.0f;
			puck->vel *= 0.7f;
			paddle2->score++;
			soundplayer->sound = state->scoreSfx;
		}
		else if (puck->pos.x + puck->halfHeight > width)
		{
			puck->pos.x = width / 2.0f;
			puck->pos.y = height / 2.0f;
			puck->vel *= 0.7f;
			paddle1->score++;
			soundplayer->sound = state->scoreSfx;
		}

		/*
			Resets the score once the a player scores 10 times.
			NOTE: the ':' character is the ascii character after the 9 character.
		*/
		if (paddle1->score == ':' || paddle2->score == ':')
		{
			ResetGameState(state, width, height);
		}

		/*
			Check to see if the puck hits either the top or bottom of the play area.
		*/
		if (puck->pos.y < 0.0f)
		{
			jr::vec2 topBoundaryNormal(0.0f, -1.0f);
			puck->pos.y = 0.0f;
			puck->vel = jr::reflect(puck->vel, topBoundaryNormal);
		}
		else if (puck->pos.y > height - puck->halfHeight)
		{
			jr::vec2 bottomBoundaryNormal(0.0f, 1.0f);
			puck->pos.y = height - 20.0f;
			puck->vel = jr::reflect(puck->vel, bottomBoundaryNormal);
		}
	}

	/*
		Render the current game state.
	*/
	if (sys->renderer)
	{
		// SuperPuck title
		DrawBitMap(sys->renderer, 0, state->titleBitMap, 0, 0);

		if (!state->mainmenu)
		{
			// Renders score.
			rect colon = CharToRect(':');
			DrawBitMapTile(sys->renderer, 0, state->fontBitMap, 500, 0, colon);
			rect p1Score = CharToRect(state->paddle1.score);
			DrawBitMapTile(sys->renderer, 0, state->fontBitMap, 500-50, 0, p1Score);
			rect p2Score = CharToRect(state->paddle2.score);
			DrawBitMapTile(sys->renderer, 0, state->fontBitMap, 500+50, 0, p2Score);

			DrawPuck(sys->renderer, &state->puck);
			DrawPaddle(sys->renderer, &state->paddle1);
			DrawPaddle(sys->renderer, &state->paddle2);
		}

		if (state->paused)
		{
			int pauseXPos = 400, pauseYPos = 500;
			DrawBitMapTile(sys->renderer, 0, state->fontBitMap, pauseXPos + 0, pauseYPos, CharToRect('P'));
			DrawBitMapTile(sys->renderer, 0, state->fontBitMap, pauseXPos + 50, pauseYPos, CharToRect('a'));
			DrawBitMapTile(sys->renderer, 0, state->fontBitMap, pauseXPos + 50 * 2, pauseYPos, CharToRect('u'));
			DrawBitMapTile(sys->renderer, 0, state->fontBitMap, pauseXPos + 50 * 3, pauseYPos, CharToRect('s'));
			DrawBitMapTile(sys->renderer, 0, state->fontBitMap, pauseXPos + 50 * 4, pauseYPos, CharToRect('e'));
		}

		if (state->mainmenu)
		{
			int pauseXPos = 400, pauseYPos = 500;
			DrawBitMapTile(sys->renderer, 0, state->fontBitMap, pauseXPos + 0, pauseYPos, CharToRect('S'));
			DrawBitMapTile(sys->renderer, 0, state->fontBitMap, pauseXPos + 50, pauseYPos, CharToRect('t'));
			DrawBitMapTile(sys->renderer, 0, state->fontBitMap, pauseXPos + 50 * 2, pauseYPos, CharToRect('a'));
			DrawBitMapTile(sys->renderer, 0, state->fontBitMap, pauseXPos + 50 * 3, pauseYPos, CharToRect('r'));
			DrawBitMapTile(sys->renderer, 0, state->fontBitMap, pauseXPos + 50 * 4, pauseYPos, CharToRect('t'));
		
			DrawBitMapTile(sys->renderer, 0, state->fontBitMap, pauseXPos + 0, pauseYPos + 70, CharToRect('Q'));
			DrawBitMapTile(sys->renderer, 0, state->fontBitMap, pauseXPos + 50, pauseYPos + 70, CharToRect('u'));
			DrawBitMapTile(sys->renderer, 0, state->fontBitMap, pauseXPos + 50 * 2, pauseYPos + 70, CharToRect('i'));
			DrawBitMapTile(sys->renderer, 0, state->fontBitMap, pauseXPos + 50 * 3, pauseYPos + 70, CharToRect('t'));
			
			int selectorYPos = startSelected ? 517 : 585;
			rect selectorRect =  rect(395, 650, selectorYPos, selectorYPos + 56);
			DrawRectangleLine(sys->renderer, 0, selectorRect, color::White);
		}
	}
}