// Super Puck check list
// Detect controllers for players
// Game modes
//		2p classic
// 		2p doubles
// 		4p ffa
//		4p team doubles
// p2,3,4 AI
// no quit to main
// material manager
// special effects
// analog stick menu move

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
	char moved;
};

void DrawPaddle(jr::Renderer* renderer, paddle_data* paddle)
{
	jr::mat3 model = jr::mat3::translation(paddle->pos.x, paddle->pos.y) * jr::mat3::rotation(paddle->angle);
	jr::vec2 tl = model * jr::vec2(-paddle->halfWidth, -paddle->halfHeight);
	jr::vec2 bl = model * jr::vec2(-paddle->halfWidth, paddle->halfHeight);
	jr::vec2 tr = model * jr::vec2(paddle->halfWidth, -paddle->halfHeight);
	jr::vec2 br = model * jr::vec2(paddle->halfWidth, paddle->halfHeight);

	DrawTriangle(renderer, 0, tl, tr, bl, paddle->color);
	DrawTriangle(renderer, 0, tr, br, bl, paddle->color);

	rect r(paddle->pos.x - paddle->halfWidth, paddle->pos.x + paddle->halfWidth, paddle->pos.y - paddle->halfHeight, paddle->pos.y + paddle->halfHeight);
	DrawRectangleLine(renderer, 0, r, jr::color::White);
}

void UpdatePaddle(paddle_data* paddle, float minY, float maxY, int16 stickX, int16 stickY, int32 deadzone)
{
	paddle->moved = false;

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
		paddle->moved = true;
	}
}

struct puck_data
{
	Color color;
	jr::vec2 pos;
	jr::vec2 vel;
	float halfWidth, halfHeight;
	float speed, maxSpeed, minSpeed;
};

void DrawPuck(jr::Renderer* renderer, puck_data* puck)
{
	rect puckRect = rect(
		puck->pos.x - puck->halfWidth, puck->pos.x + puck->halfWidth, 
		puck->pos.y - puck->halfHeight, puck->pos.y + puck->halfHeight);

	float percentSpeed = puck->speed / puck->maxSpeed;
	Color color(puck->color);
	color.a = percentSpeed;

	DrawRectangle(renderer, 0, puckRect, color);
	DrawRectangleLine(renderer, 0, puckRect, jr::color::White);
}

/*
	Puck's one and only game state.
*/
struct game_state
{
	puck_data puck;
	paddle_data paddle1, paddle2, paddle3, paddle4;

	jr::BitMap* fontBitMap;
	jr::BitMap* titleBitMap;
	jr::Sound* paddleSfx;
	jr::Sound* scoreSfx;

	bool mainmenu;
	bool paused;
};

void ResetGameState(game_state* state, float width, float height)
{
	state->paddle1.color = jr::Color(0.0f, 0.0f, 1.0f, 0.5f);
	state->paddle1.pos = vec2(15.0f, height/2.0f - 50.0f);
	state->paddle1.angle = 0.0f;
	state->paddle1.halfWidth = 10.0f;
	state->paddle1.halfHeight = 50.0f;
	state->paddle1.speed = 10.0f;
	state->paddle1.score = '0';
	state->paddle1.moved = false;

	state->paddle2.color = jr::Color(1.0f, 0.0f, 0.0f, 0.5f);
	state->paddle2.pos = vec2(width - 15.0f, height/2.0f - 50.0f);
	state->paddle2.angle = 0.0f;
	state->paddle2.halfWidth = 10.0f;
	state->paddle2.halfHeight = 50.0f;
	state->paddle2.speed = 10.0f;
	state->paddle2.score = '0';
	state->paddle2.moved = false;

	state->paddle3.color = jr::Color(1.0f, 1.0f, 0.0f, 0.5f);
	state->paddle3.pos = vec2(width/2.0f - 50.0f, 15.0f);
	state->paddle3.angle = 0.0f;
	state->paddle3.halfWidth = 50.0f;
	state->paddle3.halfHeight = 10.0f;
	state->paddle3.speed = 10.0f;
	state->paddle3.score = '0';
	state->paddle3.moved = false;

	state->paddle4.color = jr::Color(0.0f, 1.0f, 0.0f, 0.5f);
	state->paddle4.pos = vec2(width/2.0f - 50.0f, height - 15.0f);
	state->paddle4.angle = 0.0f;
	state->paddle4.halfWidth = 50.0f;
	state->paddle4.halfHeight = 10.0f;
	state->paddle4.speed = 10.0f;
	state->paddle4.score = '0';
	state->paddle4.moved = false;

	state->puck.color = jr::Color(0.0f, 1.0f, 0.0f, 0.5f);
	state->puck.pos = vec2(width / 2.0f, height / 2.0f);
	state->puck.vel = vec2(2.0f, 2.0f);
	state->puck.halfWidth = 10.0f;
	state->puck.halfHeight = 10.0f;
	state->puck.speed = 2.0f;

	state->puck.maxSpeed = 8.0f;
	state->puck.minSpeed = 2.0f;

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

	//state->titleBitMap = sys->debug->LoadBitMap("..\\data\\colortest.bmp");
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

	static bool startSelected = true;
	
	if (state->mainmenu)
	{
		if ((input->controllers[0].aButton == Released ||
			input->keyboard.enterKey == Released))
		{
			if (startSelected)
			{
				state->mainmenu = false;
			}
			else
			{
				sys->signalQuit = true;
			}
		}



		if (input->controllers[0].down == Pressed ||
			input->keyboard.wKey == Pressed ||
			input->keyboard.upKey == Pressed)
		{
			startSelected = !startSelected;
		}
	
		if (input->controllers[0].up == Pressed ||
			input->keyboard.sKey == Pressed ||
			input->keyboard.downKey == Pressed)
		{
			startSelected = !startSelected;
		}
	}

	if (!state->mainmenu)
	{
		if (input->controllers[0].start == Released ||
			input->keyboard.enterKey == Released ||
			input->keyboard.escKey == Released)
		{
			state->paused = !state->paused;
		}
	}

	if (!state->paused && !state->mainmenu)
	{
		puck_data* puck = &state->puck;
		paddle_data* paddle1 = &state->paddle1;
		paddle_data* paddle2 = &state->paddle2;
		paddle_data* paddle3 = &state->paddle3;
		paddle_data* paddle4 = &state->paddle4;

		/*
			Analog stick controls for player 1.
		*/
		controllerState &controller = input->controllers[0];
		
		UpdatePaddle(paddle1, 5.0f, height - 5.0f, controller.lStickX, controller.lStickY, 7849);
		UpdatePaddle(paddle2, 5.0f, height - 5.0f, controller.rStickY, controller.rStickY, 8689);

		paddle1->angle = 0.0f;
		paddle2->angle = 0.0f;
		if (input->controllers[0].lShoulder == Held)
		{
			paddle1->angle = JR_PI / 12.0f;
			paddle2->angle = JR_PI / 12.0f;
		}
		else if (input->controllers[0].rShoulder == Held)
		{
			paddle1->angle = JR_PI / -12.0f;
			paddle2->angle = JR_PI / -12.0f;
		}

		if (paddle1->angle > 2.0f * JR_PI)
		{
			paddle1->angle -= 2.0f * JR_PI;
			paddle2->angle -= 2.0f * JR_PI;
		}

		if (paddle1->angle < -2.0f * JR_PI)
		{
			paddle1->angle += 2.0f * JR_PI;
			paddle2->angle += 2.0f * JR_PI;
		}	

		if (input->keyboard.wKey == Pressed || input->keyboard.wKey == Held)
		{
			paddle1->pos.y -= paddle1->speed;
			if (paddle1->pos.y < 5.0f + paddle1->halfHeight)
				paddle1->pos.y = 5.0f + paddle1->halfHeight;
			if (paddle1->pos.y > height - 5.0f - paddle1->halfHeight)
				paddle1->pos.y = height - 5.0f - paddle1->halfHeight;
			paddle1->moved = true;
		}

		if (input->keyboard.sKey == Pressed || input->keyboard.sKey == Held)
		{
			paddle1->pos.y += paddle1->speed;
			if (paddle1->pos.y < 5.0f + paddle1->halfHeight)
				paddle1->pos.y = 5.0f + paddle1->halfHeight;
			if (paddle1->pos.y > height - 5.0f - paddle1->halfHeight)
				paddle1->pos.y = height - 5.0f - paddle1->halfHeight;
			paddle1->moved = true;
		}

		if (input->keyboard.upKey == Pressed || input->keyboard.upKey == Held)
		{
			paddle2->pos.y -= paddle2->speed;
			if (paddle2->pos.y < 5.0f + paddle2->halfHeight)
				paddle2->pos.y = 5.0f + paddle2->halfHeight;
			if (paddle2->pos.y > height - 5.0f - paddle2->halfHeight)
				paddle2->pos.y = height - 5.0f - paddle2->halfHeight;
			paddle2->moved = true;
		}

		if (input->keyboard.downKey == Pressed || input->keyboard.downKey == Held)
		{
			paddle2->pos.y += paddle2->speed;
			if (paddle2->pos.y < 5.0f + paddle2->halfHeight)
				paddle2->pos.y = 5.0f + paddle2->halfHeight;
			if (paddle2->pos.y > height - 5.0f - paddle2->halfHeight)
				paddle2->pos.y = height - 5.0f - paddle2->halfHeight;
			paddle2->moved = true;
		}

		if (input->keyboard.aKey == Pressed || input->keyboard.aKey == Held)
		{
			paddle3->pos.x -= paddle3->speed;
			if (paddle3->pos.x < 5.0f + paddle3->halfWidth)
				paddle3->pos.x = 5.0f + paddle3->halfWidth;
			if (paddle3->pos.x > width - 5.0f - paddle3->halfWidth)
				paddle3->pos.x = width - 5.0f - paddle3->halfWidth;
			paddle3->moved = true;
		}

		if (input->keyboard.dKey == Pressed || input->keyboard.dKey == Held)
		{
			paddle3->pos.x += paddle3->speed;
			if (paddle3->pos.x < 5.0f + paddle3->halfWidth)
				paddle3->pos.x = 5.0f + paddle3->halfWidth;
			if (paddle3->pos.x > width - 5.0f - paddle3->halfWidth)
				paddle3->pos.x = width - 5.0f - paddle3->halfWidth;
			paddle3->moved = true;
		}

		if (input->keyboard.leftKey == Pressed || input->keyboard.leftKey == Held)
		{
			paddle4->pos.x -= paddle4->speed;
			if (paddle4->pos.x < 5.0f + paddle4->halfWidth)
				paddle4->pos.x = 5.0f + paddle4->halfWidth;
			if (paddle4->pos.x > width - 5.0f - paddle4->halfWidth)
				paddle4->pos.x = width - 5.0f - paddle4->halfWidth;
			paddle4->moved = true;
		}

		if (input->keyboard.rightKey == Pressed || input->keyboard.rightKey == Held)
		{
			paddle4->pos.x += paddle4->speed;
			if (paddle4->pos.x < 5.0f + paddle4->halfWidth)
				paddle4->pos.x = 5.0f + paddle4->halfWidth;
			if (paddle4->pos.x > width - 5.0f - paddle4->halfWidth)
				paddle4->pos.x = width - 5.0f - paddle4->halfWidth;
			paddle4->moved = true;
		}

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
			if (paddle1->moved)
				puck->speed = jr::minf(puck->speed + 0.5f, puck->maxSpeed);

			jr::vec2 paddleNormal = jr::mat3::rotation(paddle1->angle) * jr::vec2(1.0f, 0.0f);
			puck->vel = jr::reflect(jr::normalize(puck->vel) * puck->speed, paddleNormal);
			puck->color = paddle1->color;
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
			puck->speed = jr::minf(puck->speed + 0.5f, puck->maxSpeed);

			jr::vec2 paddleNormal = jr::mat3::rotation(paddle1->angle) * jr::vec2(-1.0f, 0.0f);
			puck->vel = jr::reflect(jr::normalize(puck->vel) * puck->speed, paddleNormal);
			puck->color = paddle2->color;
			soundplayer->sound = state->paddleSfx;
		}

		if (puck->pos.y - puck->halfHeight < paddle3->pos.y + paddle3->halfHeight
			&& puck->pos.x - puck->halfHeight < paddle3->pos.x + paddle3->halfWidth
			&& puck->pos.x + puck->halfHeight > paddle3->pos.x - paddle3->halfWidth)
		{
			puck->pos.y = paddle3->pos.y + paddle3->halfHeight + 15.0f;
			puck->speed = jr::minf(puck->speed + 0.5f, puck->maxSpeed);

			jr::vec2 paddleNormal = jr::mat3::rotation(paddle1->angle) * jr::vec2(0.0f, 1.0f);
			puck->vel = jr::reflect(jr::normalize(puck->vel) * puck->speed, paddleNormal);
			puck->color = paddle3->color;
			soundplayer->sound = state->paddleSfx;
		}

		if (puck->pos.y + puck->halfHeight > paddle4->pos.y - paddle4->halfHeight
			&& puck->pos.x - puck->halfHeight < paddle4->pos.x + paddle4->halfWidth
			&& puck->pos.x + puck->halfHeight > paddle4->pos.x - paddle4->halfWidth)
		{
			puck->pos.y = paddle4->pos.y - paddle4->halfHeight - 15.0f;
			puck->speed = jr::minf(puck->speed + 0.5f, puck->maxSpeed);

			jr::vec2 paddleNormal = jr::mat3::rotation(paddle1->angle) * jr::vec2(0.0f, -1.0f);
			puck->vel = jr::reflect(jr::normalize(puck->vel) * puck->speed, paddleNormal);
			puck->color = paddle4->color;
			soundplayer->sound = state->paddleSfx;
		}

		/*
			Check to see if the puck scored for either player 1 or player 2.
		*/
		if (puck->pos.x - puck->halfWidth < 0.0f)
		{
			if (puck->color == paddle2->color)
			{
				puck->pos.x = width / 2.0f;
				puck->pos.y = height / 2.0f;
				puck->vel *= 0.7f;
				paddle2->score++;
				soundplayer->sound = state->scoreSfx;
			}
			else
			{
				puck->pos.x = puck->halfWidth;
				puck->vel = jr::reflect(puck->vel, jr::vec2(1.0f, 0.0f));
			}
		}
		else if (puck->pos.x + puck->halfWidth > width)
		{	
			if(puck->color == paddle1->color)
			{
				puck->pos.x = width / 2.0f;
				puck->pos.y = height / 2.0f;
				puck->vel *= 0.7f;
				paddle1->score++;
				soundplayer->sound = state->scoreSfx;
			}
			else
			{
				puck->pos.x = width - puck->halfWidth;
				puck->vel = jr::reflect(puck->vel, jr::vec2(1.0f, 0.0f));
			}
		}
		else if (puck->pos.y - puck->halfHeight < 0.0f)
		{
			if (puck->color == paddle4->color)
			{
				puck->pos.x = width / 2.0f;
				puck->pos.y = height / 2.0f;
				puck->vel *= 0.7f;
				paddle4->score++;
				soundplayer->sound = state->scoreSfx;
			}
			else
			{
				puck->pos.y = puck->halfHeight;
				puck->vel = jr::reflect(puck->vel, jr::vec2(0.0f, -1.0f));
			}
		}
		else if (puck->pos.y + puck->halfHeight > height)
		{
			if(puck->color == paddle3->color)
			{
				puck->pos.x = width / 2.0f;
				puck->pos.y = height / 2.0f;
				puck->vel *= 0.7f;
				paddle3->score++;
				soundplayer->sound = state->scoreSfx;
			}
			else
			{
				puck->pos.y = height - puck->halfHeight;
				puck->vel = jr::reflect(puck->vel, jr::vec2(0.0f, 1.0f));
			}
		}

		/*
			Resets the score once the a player scores 10 times.
			NOTE: the ':' character is the ascii character after the 9 character.
		*/
		if (paddle1->score == ':' || paddle2->score == ':')
		{
			ResetGameState(state, width, height);
		}
	}

	/*
		Render the current game state.
	*/
	if (sys->renderer)
	{
		// SuperPuck title
		DrawBitMap(sys->renderer, 0, state->titleBitMap, 0, 0);

		if (state->paused)
		{
			int pauseXPos = 400, pauseYPos = 500;
			DrawBitMapTile(sys->renderer, 0, state->fontBitMap, pauseXPos + 0, pauseYPos, CharToRect('P'));
			DrawBitMapTile(sys->renderer, 0, state->fontBitMap, pauseXPos + 50, pauseYPos, CharToRect('a'));
			DrawBitMapTile(sys->renderer, 0, state->fontBitMap, pauseXPos + 50 * 2, pauseYPos, CharToRect('u'));
			DrawBitMapTile(sys->renderer, 0, state->fontBitMap, pauseXPos + 50 * 3, pauseYPos, CharToRect('s'));
			DrawBitMapTile(sys->renderer, 0, state->fontBitMap, pauseXPos + 50 * 4, pauseYPos, CharToRect('e'));
		}

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
			DrawPaddle(sys->renderer, &state->paddle3);
			DrawPaddle(sys->renderer, &state->paddle4);
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