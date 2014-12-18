#define _WIN32_DCOM
#define _CRT_SECURE_NO_DEPRECATE

#include "puck.h"
#include "puck_math.h"
#include "puck_mem.h"
#include <windows.h>
#include "GL\gl3w.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "c:\Program Files (x86)\Microsoft DirectX SDK (June 2010)\Include\XAudio2.h"
#include <xinput.h>
#include <math.h>
#include <new>
#include <assert.h>

#pragma comment (lib, "xinput9_1_0.lib")
#pragma comment (lib, "opengl32.lib")
#pragma comment (lib, "user32.lib")
#pragma comment (lib, "gdi32.lib")
#pragma comment (lib, "ole32.lib")

using namespace puck;

HWND hwnd;
HDC hdc;
HGLRC hglrc;

GLuint program;
GLuint vertexArray;
GLuint vertexBuffer;
GLuint transformBuffer;
GLuint elementBuffer;
GLint vertexLoc;
GLint transformLoc;
GLint cameraLoc;

IXAudio2* xaudio2;
IXAudio2MasteringVoice* masterVoice;
IXAudio2SourceVoice* sourceVoice;
BYTE* soundData;

bool running = false;

game_data game;
void* memBlock = nullptr;

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
GLuint CreateBasicShader(const char* vertexCode, const char* fragmentCode);
GLuint CompileShader(GLenum type, const char* code);
size_t LoadTextFile(const char* fileName, char* &buffer);
void DrawRectangle(vec3 position, float xScale, float yScale);

void RenderStuff();
bool RectIntersection(rect r1, rect r2);

mat4 camera = OrthoProjection(0.0f, windowWidth, 0.0f, -windowHeight, -0.1f, 10000.0f);

int WinMain(HINSTANCE instance, HINSTANCE prevInstance, LPSTR cmdLine, int showCmd)
{
	WNDCLASS wc = {}; 
	
	wc.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc   = WndProc;
	wc.hInstance     = instance;
	wc.lpszClassName = "PuckWindowClass";
	
	if (!RegisterClass(&wc))
		return 1;

	hwnd = CreateWindowEx(
		0, 									// ExStyle
		wc.lpszClassName, 					// Class Name
		"Puck",								// Window Name
		WS_OVERLAPPEDWINDOW | WS_VISIBLE,	// Style
		0, 0, 1024, 768,					// X, Y, Width, Height
		0,									// HWND Parent
		0,									// Menu
		instance,							// Instance
		0);									// lpParam

	memBlock = VirtualAlloc((void*)0x0000000200000000, MEGABYTE(5), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	assert(memBlock);
	void* memHead = (void*)((uintptr_t)memBlock + sizeof(MemManager));
	size_t memBlockSize = MEGABYTE(5) - sizeof(MemManager);

	game.mem = new ((MemManager*)memBlock) MemManager(memHead, memBlockSize, MEGABYTE(1));
	game.input = new (game.mem->Alloc(sizeof(game_input))) game_input;
	game.renderer = new (game.mem->Alloc(sizeof(game_renderer))) game_renderer;
	game.state = new (game.mem->Alloc(sizeof(game_state))) game_state;
	game.soundplayer = new (game.mem->Alloc(sizeof(game_soundplayer))) game_soundplayer;

	InitializeGameState(game.state);
	game.soundplayer->test = false;

	running = true;
	while (running)
	{
		MSG msg = {};
		while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
			{
				running = false;
			}

			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		for(int i = 0; i < 4; ++i)
		{
			XINPUT_STATE controllerState;
			ZeroMemory(&controllerState, sizeof(XINPUT_STATE));

			DWORD result = XInputGetState(i, &controllerState);
			if(result == ERROR_SUCCESS)
			{
				XINPUT_GAMEPAD *pad = &controllerState.Gamepad;

				game.input->controllers[i].up = (pad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
				game.input->controllers[i].down = (pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
				game.input->controllers[i].left = (pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
				game.input->controllers[i].right = (pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);
				game.input->controllers[i].start = (pad->wButtons & XINPUT_GAMEPAD_START);
				game.input->controllers[i].back = (pad->wButtons & XINPUT_GAMEPAD_BACK);
				game.input->controllers[i].lThumb = (pad->wButtons & XINPUT_GAMEPAD_LEFT_THUMB);
				game.input->controllers[i].rThumb = (pad->wButtons & XINPUT_GAMEPAD_RIGHT_THUMB);
				game.input->controllers[i].lShoulder = (pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER);
				game.input->controllers[i].rShoulder = (pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER);
				game.input->controllers[i].aButton = (pad->wButtons & XINPUT_GAMEPAD_A);
				game.input->controllers[i].bButton = (pad->wButtons & XINPUT_GAMEPAD_B);
				game.input->controllers[i].xButton = (pad->wButtons & XINPUT_GAMEPAD_X);
				game.input->controllers[i].yButton = (pad->wButtons & XINPUT_GAMEPAD_Y);

				game.input->controllers[i].lStickX = pad->sThumbLX;
				game.input->controllers[i].lStickY = pad->sThumbLY;
				game.input->controllers[i].rStickX = pad->sThumbRX;
				game.input->controllers[i].rStickY = pad->sThumbRY;
			}
		}

		RenderStuff();
		GameUpdate(&game);

		if (game.soundplayer->test)
		{
			game.soundplayer->test = false;

			XAUDIO2_BUFFER buffer = {0};
			buffer.AudioBytes = 2 * 5 * 44100;
			buffer.pAudioData = soundData;
			buffer.Flags = XAUDIO2_END_OF_STREAM;
			buffer.PlayBegin = 0;
			buffer.PlayLength = 0.25 * 44100;

			HRESULT hr = sourceVoice->SubmitSourceBuffer(&buffer);
			if (FAILED(hr))
				OutputDebugStringA("Failed to play sound.");
		}

	}
	return 0;
}

void RenderStuff()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glBindBuffer(GL_ARRAY_BUFFER, transformBuffer);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float) * 16 * game.renderer->numTransforms, game.renderer->transforms);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementBuffer);
	glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, sizeof(short) * game.renderer->numIndicies, game.renderer->indices);

	glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0, game.renderer->numTransforms);

	game.renderer->numIndicies = 0;
	game.renderer->numTransforms = 0;

	SwapBuffers(hdc);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message)
	{
	case WM_SIZE:
		{
			glViewport(0, 0, LOWORD(lParam), HIWORD(lParam));
			OutputDebugStringA("WM_SIZE\n");
		}
		break;
	case WM_DESTROY:
		{
			OutputDebugStringA("WM_DESTROY\n");
		}
		break;
	case WM_CLOSE:
		{
			running = false;
			OutputDebugStringA("WM_CLOSE\n");
			wglDeleteContext(hglrc);
			if (xaudio2)
				xaudio2->Release();
			CoUninitialize();
			PostQuitMessage(0);
		}
		break;
	case WM_CREATE:
		{
			OutputDebugStringA("WM_CREATE\n");
			PIXELFORMATDESCRIPTOR pfd =
			{
				sizeof(PIXELFORMATDESCRIPTOR),
				1,
				PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,    //Flags
				PFD_TYPE_RGBA,            //The kind of framebuffer. RGBA or palette.
				32,                        //Colordepth of the framebuffer.
				0, 0, 0, 0, 0, 0,
				0,
				0,
				0,
				0, 0, 0, 0,
				24,                        //Number of bits for the depthbuffer
				8,                        //Number of bits for the stencilbuffer
				0,                        //Number of Aux buffers in the framebuffer.
				PFD_MAIN_PLANE,
				0,
				0, 0, 0
			};

			hdc = GetDC(hWnd);

			int  pixelFormat;
			pixelFormat = ChoosePixelFormat(hdc, &pfd); 
			SetPixelFormat(hdc,pixelFormat, &pfd);

			hglrc = wglCreateContext(hdc);
			wglMakeCurrent (hdc, hglrc);
			int result = gl3wInit();
			glClearColor(0.0f, 0.0f, 0.5f, 1.0f);

			char *vertShader, *fragShader;
			LoadTextFile("basic.vert", vertShader);
			LoadTextFile("basic.frag", fragShader);
			program = CreateBasicShader(vertShader, fragShader);
			
			delete[] vertShader;
			delete[] fragShader;

			glUseProgram(program);

			vertexLoc = glGetAttribLocation(program, "vertex");
			transformLoc = glGetAttribLocation(program, "transform");
			cameraLoc = glGetUniformLocation(program, "camera");

			glGenVertexArrays(1, &vertexArray);
			glBindVertexArray(vertexArray);
		
			glGenBuffers(1, &vertexBuffer);
			glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
			glBufferData(GL_ARRAY_BUFFER, 10000 * sizeof(vec3), nullptr, GL_STATIC_DRAW);
			glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vec3) * 4, squareVerts);
			glVertexAttribPointer(vertexLoc, 3, GL_FLOAT, GL_FALSE, 0, 0);
			glEnableVertexAttribArray(vertexLoc);

			glGenBuffers(1, &transformBuffer);
			glBindBuffer(GL_ARRAY_BUFFER, transformBuffer);
			glBufferData(GL_ARRAY_BUFFER, 1000 * sizeof(mat4), nullptr, GL_DYNAMIC_DRAW);
			for (int i = 0; i < 4; i++)
			{
				glEnableVertexAttribArray(transformLoc + i);
				glVertexAttribPointer(transformLoc + i, 4, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 16, (const GLvoid*)(sizeof(GLfloat) * i * 4));
				glVertexAttribDivisor(transformLoc + i, 1);
			}

			glBindBuffer(GL_ARRAY_BUFFER, 0);

			glGenBuffers(1, &elementBuffer);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementBuffer);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, 10000 * sizeof(short), nullptr, GL_DYNAMIC_DRAW);

			glUniformMatrix4fv(cameraLoc, 1, GL_FALSE, (camera.data));

			CoInitializeEx(NULL, COINIT_MULTITHREADED);

			HRESULT hr;
			hr = XAudio2Create(&xaudio2, 0, XAUDIO2_DEFAULT_PROCESSOR);
			hr = xaudio2->CreateMasteringVoice(&masterVoice);

			WAVEFORMATEX waveformat;
			waveformat.wFormatTag = WAVE_FORMAT_PCM;
			waveformat.nChannels = 1;
			waveformat.nSamplesPerSec = 44100;
			waveformat.nAvgBytesPerSec = 44100 * 2;
			waveformat.nBlockAlign = 2;
			waveformat.wBitsPerSample = 16;
			waveformat.cbSize = 0;

			hr = xaudio2->CreateSourceVoice(&sourceVoice, &waveformat);

			soundData = new BYTE[5 * 2 * 44100];
			sourceVoice->Start();
			for (int index = 0, second = 0; second < 5; second++)
			{
				for (int cycle = 0; cycle < 441; cycle++)
				{
					for (int sample = 0; sample < 100; sample++)
					{
						short value = sample < 50 ? 32767 : -32768;
						soundData[index++] = value & 0xFF;
						soundData[index++] = (value >> 8) & 0xFF;
					}
				}
			}
		}
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

GLuint CreateBasicShader(const char* vertexCode, const char* fragmentCode)
{
	GLuint program, vertShader, fragShader;
	vertShader = CompileShader(GL_VERTEX_SHADER, vertexCode);
	fragShader = CompileShader(GL_FRAGMENT_SHADER, fragmentCode);

	program = glCreateProgram();
	glAttachShader(program, vertShader);
	glAttachShader(program, fragShader);
	glLinkProgram(program);

	glDeleteShader(vertShader);
	glDeleteShader(fragShader);
	
	return program;
}

GLuint CompileShader(GLenum type, const char* code)
{
	GLuint handle = glCreateShader(type);
	glShaderSource(handle, 1, &code, nullptr);
	glCompileShader(handle);

	GLint status;
	glGetShaderiv(handle, GL_COMPILE_STATUS, &status);
	if (status == GL_FALSE)
	{
		GLint infoLogLength;
		glGetShaderiv(handle, GL_INFO_LOG_LENGTH, &infoLogLength);

		char* errors = new char[infoLogLength + 1];
		memset(errors, '\0', infoLogLength + 1);

		glGetShaderInfoLog(handle, infoLogLength, nullptr, errors);

		MessageBoxA(hwnd, errors, "Error Compiling Shader", MB_OK);
		delete errors;
	}

	return handle;
}

size_t LoadTextFile(const char* fileName, char* &buffer)
{
	size_t read = 0;
	FILE* file = nullptr;
	errno_t error = fopen_s(&file, fileName, "r");

	if (error == 0)
	{
		fseek(file, 0, SEEK_END);
		size_t size = ftell(file);
		rewind(file);

		buffer = new char[size + 1];
		memset(buffer, 0x00, size + 1);
		read = fread_s(buffer, size + 1, 1, size, file);
		buffer[read] = 0x00;

		if (read == 0)
		{
			delete[] buffer;
		}

		fclose(file);
	}

	return read;
}

