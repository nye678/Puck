#define _WIN32_DCOM
#define _CRT_SECURE_NO_DEPRECATE

#include "puck_gamecommon.h"
#include "puck.h"
#include "jr_memmanager.h"
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

static int windowWidth = 1024;
static int windowHeight = 768;

HWND hwnd;
HDC hdc;
HGLRC hglrc;

GLuint program;
GLuint vertexArray;
GLuint vertexBuffer;
GLuint pbo[2];
int pboIndex = 0;
jr::Renderer* renderer[2];
jr::RenderBuffer* renderBuffer;

GLuint texture;
GLint texLoc;

IXAudio2* xaudio2;
IXAudio2MasteringVoice* masterVoice;
IXAudio2SourceVoice* sourceVoice;
game_soundplayer* soundplayer[2];

bool running = false;
CONDITION_VARIABLE TriggerGameUpdate;
CRITICAL_SECTION GameUpdateLock;

debug_tools debug;

Systems sys;

void* memBlock = nullptr;

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
GLuint CreateBasicShader(const char* vertexCode, const char* fragmentCode);
GLuint CompileShader(GLenum type, const char* code);
size_t LoadTextFile(const char* fileName, char* &buffer);
DWORD WINAPI GameUpdateProc(void* param);

jr::BitMap* DebugLoadBitMap(const char* filepath)
{
	return ReadBMP(sys.mem, filepath);
}

jr::Sound* DebugLoadSound(const char* filepath)
{
	return ReadWave(sys.mem, filepath);
}

int WinMain(HINSTANCE instance, HINSTANCE prevInstance, LPSTR cmdLine, int showCmd)
{
	WNDCLASSEX wc = {}; 
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc   = WndProc;
	wc.hInstance     = instance;
	wc.lpszClassName = "PuckWindowClass";
	
	if (!RegisterClassEx(&wc))
		return 1;

	hwnd = CreateWindowEx(
		0, 									// ExStyle
		wc.lpszClassName, 					// Class Name
		"Puck",								// Window Name
		WS_OVERLAPPEDWINDOW | WS_VISIBLE,	// Style
		0, 0, windowWidth, windowHeight,					// X, Y, Width, Height
		0,									// HWND Parent
		0,									// Menu
		instance,							// Instance
		0);						// lpParam

	memBlock = VirtualAlloc((void*)0x0000000200000000, MEGABYTE(50), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	assert(memBlock);
	void* memHead = (void*)((uintptr_t)memBlock + sizeof(MemManager));
	size_t memBlockSize = MEGABYTE(50) - sizeof(MemManager);
	
	sys.mem = new ((MemManager*)memBlock) MemManager(memHead, memBlockSize, MEGABYTE(20));
	
	renderBuffer = new (sys.mem->Alloc(sizeof(jr::RenderBuffer))) jr::RenderBuffer;
	renderBuffer->width = windowWidth;
	renderBuffer->height = windowHeight;
	renderBuffer->layers = 1;

	renderer[0] = CreateRenderer(sys.mem, 64, 64 * sizeof(jr::DrawBitMapParams));
	renderer[0]->bufferWidth = windowWidth;
	renderer[0]->bufferHeight = windowHeight;
	renderer[1] = CreateRenderer(sys.mem, 64, 64 * sizeof(jr::DrawBitMapParams));
	renderer[1]->bufferWidth = windowWidth;
	renderer[1]->bufferHeight = windowHeight;
	sys.renderer = renderer[0];

	sys.input = new (sys.mem->Alloc(sizeof(game_input))) game_input;
	//sys.state = new (sys.mem->Alloc(sizeof(game_state))) game_state;
	
	soundplayer[0] = (game_soundplayer*)sys.mem->Alloc(sizeof(game_soundplayer));
	soundplayer[0]->sound = nullptr;
	soundplayer[1] = (game_soundplayer*)sys.mem->Alloc(sizeof(game_soundplayer));
	soundplayer[1]->sound = nullptr;
	sys.soundplayer = soundplayer[0];

	debug.LoadBitMap = DebugLoadBitMap;
	debug.LoadSound = DebugLoadSound;
	sys.debug = &debug;

	InitializeGame(&sys);

	running = true;

	InitializeConditionVariable(&TriggerGameUpdate);
	InitializeCriticalSection(&GameUpdateLock);

	DWORD id;
	HANDLE gameUpdateThread = CreateThread(nullptr, 0, GameUpdateProc, (void*)(&sys), 0, &id);

	while (running)
	{
		MSG msg = {};
		while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
			{
				EnterCriticalSection(&GameUpdateLock);
				running = false;
				LeaveCriticalSection(&GameUpdateLock);
			}

			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		int nextIndex = pboIndex;
		pboIndex = (pboIndex + 1) % 2;

		ResetRenderer(renderer[nextIndex]);

		// Trigger the game update thread.
		EnterCriticalSection(&GameUpdateLock);
		sys.renderer = renderer[nextIndex];
		sys.soundplayer = soundplayer[nextIndex];
		LeaveCriticalSection(&GameUpdateLock);
		WakeConditionVariable(&TriggerGameUpdate);

		// Begin rendering.
		if (soundplayer[pboIndex]->sound)
		{
			jr::Sound* sound = soundplayer[pboIndex]->sound;

			XAUDIO2_BUFFER buffer = {0};
			buffer.AudioBytes = sound->audioBytes;
			buffer.pAudioData = sound->buffer;
			buffer.Flags = XAUDIO2_END_OF_STREAM;
			buffer.PlayBegin = 0;
			buffer.PlayLength = sound->audioBytes / 2;

			HRESULT hr = sourceVoice->SubmitSourceBuffer(&buffer);
			if (FAILED(hr))
				OutputDebugStringA("Failed to play sound.");
			
			soundplayer[pboIndex]->sound = nullptr;
		}

		glClear(GL_COLOR_BUFFER_BIT);

		// Uploads the previous pbo to the texture.
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo[pboIndex]);
		glBindTexture(GL_TEXTURE_2D, texture);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 1024, 768, GL_RGBA, GL_UNSIGNED_BYTE, (void*)0);

		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		SwapBuffers(hdc);

		// Map the next pbo to the render buffer.
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo[nextIndex]);
		glBufferData(GL_PIXEL_UNPACK_BUFFER, sizeof(uint8_t) * 1024 * 768 * 4, 0, GL_STREAM_DRAW);
		
		// Use the previous renderer queue to render the next frame.
		renderBuffer->buffer[0] = (uint32_t*)glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);
		if (renderBuffer->buffer[0] != nullptr)
		{
			RenderFrame(renderer[pboIndex], renderBuffer);
		}

		glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

	}

	WaitForSingleObject(gameUpdateThread, INFINITE);

	return 0;
}

DWORD WINAPI GameUpdateProc(void* param)
{
	Systems* sys = (Systems*)param;

	EnterCriticalSection(&GameUpdateLock);
	
	while (true)
	{
		for(int i = 0; i < 4; ++i)
		{
			XINPUT_STATE controllerState;
			ZeroMemory(&controllerState, sizeof(XINPUT_STATE));

			DWORD result = XInputGetState(i, &controllerState);
			if(result == ERROR_SUCCESS)
			{
				XINPUT_GAMEPAD *pad = &controllerState.Gamepad;

				sys->input->controllers[i].up = (pad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
				sys->input->controllers[i].down = (pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
				sys->input->controllers[i].left = (pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
				sys->input->controllers[i].right = (pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);
				sys->input->controllers[i].start = (pad->wButtons & XINPUT_GAMEPAD_START);
				sys->input->controllers[i].back = (pad->wButtons & XINPUT_GAMEPAD_BACK);
				sys->input->controllers[i].lThumb = (pad->wButtons & XINPUT_GAMEPAD_LEFT_THUMB);
				sys->input->controllers[i].rThumb = (pad->wButtons & XINPUT_GAMEPAD_RIGHT_THUMB);
				sys->input->controllers[i].lShoulder = (pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER);
				sys->input->controllers[i].rShoulder = (pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER);
				sys->input->controllers[i].aButton = (pad->wButtons & XINPUT_GAMEPAD_A);
				sys->input->controllers[i].bButton = (pad->wButtons & XINPUT_GAMEPAD_B);
				sys->input->controllers[i].xButton = (pad->wButtons & XINPUT_GAMEPAD_X);
				sys->input->controllers[i].yButton = (pad->wButtons & XINPUT_GAMEPAD_Y);

				sys->input->controllers[i].lStickX = pad->sThumbLX;
				sys->input->controllers[i].lStickY = pad->sThumbLY;
				sys->input->controllers[i].rStickX = pad->sThumbRX;
				sys->input->controllers[i].rStickY = pad->sThumbRY;
			}
		}

		GameUpdate(sys);
		SleepConditionVariableCS(&TriggerGameUpdate, &GameUpdateLock, INFINITE);

		if (!running)
		{
			break;
		}
	}
	
	LeaveCriticalSection(&GameUpdateLock);

	return 0;
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
			glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

			char *vertShader, *fragShader;
			LoadTextFile("basic.vert", vertShader);
			LoadTextFile("basic.frag", fragShader);
			program = CreateBasicShader(vertShader, fragShader);
			
			delete[] vertShader;
			delete[] fragShader;

			glUseProgram(program);

			texLoc = glGetUniformLocation(program, "tex");

			glGenVertexArrays(1, &vertexArray);
			glBindVertexArray(vertexArray);
		
			uint8_t* blarg = new uint8_t[1024 * 768 * 4];
			memset(blarg, 0x4F, 1024 * 768 * 4);

			glGenBuffers(2, pbo);
			glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo[1]);
			glBufferData(GL_PIXEL_UNPACK_BUFFER, sizeof(uint8_t) * 1024 * 768 * 4, blarg, GL_STREAM_DRAW);
			glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo[0]);
			glBufferData(GL_PIXEL_UNPACK_BUFFER, sizeof(uint8_t) * 1024 * 768 * 4, blarg, GL_STREAM_DRAW);
			int blerg = glGetError();
			delete [] blarg;

			glUniform1i(texLoc, 0);
			glActiveTexture(GL_TEXTURE0);
			glGenTextures(1, &texture);
			glBindTexture(GL_TEXTURE_2D, texture);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			blerg = glGetError();

			glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGB8, 1024, 768);
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 1024, 768, GL_RGBA, GL_UNSIGNED_BYTE, (void*)0);
			blerg = glGetError();

			glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

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
			sourceVoice->Start();
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

