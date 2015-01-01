#define _WIN32_DCOM
#define _CRT_SECURE_NO_DEPRECATE

#include <windows.h>
#include "GL\gl3w.h"
#include <stdio.h>
#include <string.h>
#include "c:\Program Files (x86)\Microsoft DirectX SDK (June 2010)\Include\XAudio2.h"
#include <xinput.h>
#include <new>
#include <assert.h>
#include "puck_gamecommon.h"
#include "jr_pointerutils.h"
#include "jr_memmanager.h"

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

typedef void (*InitializeGameFunc)(Systems* sys);
InitializeGameFunc InitializeGame;

typedef void (*GameUpdateFunc)(Systems* sys);
GameUpdateFunc GameUpdate;

HMODULE gamelib;

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
	/*
		Create the game window.
	*/
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

	/*
		Load the game dll.
	*/
	CopyFile("puck.dll", "puck_temp.dll", false);
	
	HANDLE gamelibFileHandle = CreateFile("puck.dll", GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	FILETIME gamelibFileTime = {};
	GetFileTime(gamelibFileHandle, NULL, NULL, &gamelibFileTime);
	CloseHandle(gamelibFileHandle);

	gamelib = LoadLibrary("puck_temp.dll");
	InitializeGame = (InitializeGameFunc)GetProcAddress(gamelib, "InitializeGame");
	GameUpdate = (GameUpdateFunc)GetProcAddress(gamelib, "GameUpdate");

	/*
		Alloc a big block of memory to use.
	*/
	memBlock = VirtualAlloc((void*)0x0000000200000000, MEGABYTE(500), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	assert(memBlock);
	void* memHead = (void*)((uintptr_t)memBlock + sizeof(jr::MemManager));
	size_t memBlockSize = MEGABYTE(500) - sizeof(jr::MemManager);
	
	sys.mem = new ((jr::MemManager*)memBlock) jr::MemManager(memHead, memBlockSize, MEGABYTE(400));
	
	/*
		Set up the rendering structs.
	*/
	renderBuffer = new (sys.mem->Alloc(sizeof(jr::RenderBuffer))) jr::RenderBuffer;
	renderBuffer->width = windowWidth;
	renderBuffer->height = windowHeight;
	renderBuffer->layers = 1;

	renderer[0] = CreateRenderer(sys.mem, 64, 64 * 50);
	renderer[0]->bufferWidth = windowWidth;
	renderer[0]->bufferHeight = windowHeight;
	renderer[1] = CreateRenderer(sys.mem, 64, 64 * 50);
	renderer[1]->bufferWidth = windowWidth;
	renderer[1]->bufferHeight = windowHeight;
	sys.renderer = renderer[0];

	/*
		Create the input struct.
	*/
	sys.input = new (sys.mem->Alloc(sizeof(game_input))) game_input;
	
	/*
		Create the sound structs.
	*/
	soundplayer[0] = (game_soundplayer*)sys.mem->Alloc(sizeof(game_soundplayer));
	soundplayer[0]->sound = nullptr;
	soundplayer[1] = (game_soundplayer*)sys.mem->Alloc(sizeof(game_soundplayer));
	soundplayer[1]->sound = nullptr;
	sys.soundplayer = soundplayer[0];

	/*
		Create debug tools structs.
	*/
	debug.LoadBitMap = DebugLoadBitMap;
	debug.LoadSound = DebugLoadSound;
	sys.debug = &debug;

	/*
		Initialize the game and game update thread.
	*/
	InitializeGame(&sys);

	running = true;

	InitializeConditionVariable(&TriggerGameUpdate);
	InitializeCriticalSection(&GameUpdateLock);

	DWORD id;
	HANDLE gameUpdateThread = CreateThread(nullptr, 0, GameUpdateProc, (void*)(&sys), 0, &id);

	/*
		Main Game Loop
	*/
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

		/*
			Update buffer index. 0 -> 1 or 1 -> 0.
		*/
		int nextIndex = pboIndex;
		pboIndex = (pboIndex + 1) % 2;

		gamelibFileHandle = CreateFile("puck.dll", GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		FILETIME gamelibCurrentTime = {};
		GetFileTime(gamelibFileHandle, NULL, NULL, &gamelibCurrentTime);
		CloseHandle(gamelibFileHandle);

		if (CompareFileTime(&gamelibCurrentTime, &gamelibFileTime) > 0)
		{
			/*
				Hot reload game dll
			*/
			EnterCriticalSection(&GameUpdateLock);

			FreeLibrary(gamelib);
			if (!CopyFile("puck.dll", "puck_temp.dll", false))
			{
				int error = GetLastError();
				OutputDebugStringA("Failed to copy game lib.");
			}
			
			gamelib = LoadLibrary("puck_temp.dll");
			InitializeGame = (InitializeGameFunc)GetProcAddress(gamelib, "InitializeGame");
			GameUpdate = (GameUpdateFunc)GetProcAddress(gamelib, "GameUpdate");
			
			LeaveCriticalSection(&GameUpdateLock);	
		
			memcpy(&gamelibFileTime, &gamelibCurrentTime, sizeof(FILETIME));

			/*
				Drop a frame, to work around a bug involving shared code
				between the game exe and dll. Render command buffer uses
				function pointers but those pointers may be incorrect after
				reloading the dll.
			*/
			ResetRenderer(renderer[pboIndex]);
		}


		/*
			Trigger the game update thread.
		*/
		ResetRenderer(renderer[nextIndex]);
		EnterCriticalSection(&GameUpdateLock);
		sys.renderer = renderer[nextIndex];
		sys.soundplayer = soundplayer[nextIndex];
		LeaveCriticalSection(&GameUpdateLock);
		WakeConditionVariable(&TriggerGameUpdate);

		/*
			Render the current frame using the command queue generated during the previous frame.
		*/
		glClear(GL_COLOR_BUFFER_BIT);
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo[0]);
		glBufferData(GL_PIXEL_UNPACK_BUFFER, sizeof(float) * windowWidth * windowHeight * 4, 0, GL_STREAM_DRAW);
		
		renderBuffer->buffer[0] = (jr::Color*)glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_READ_WRITE);
		if (renderBuffer->buffer[0] != nullptr)
		{
			RenderFrame(renderer[pboIndex], renderBuffer);
		}

		glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);

		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, windowWidth, windowHeight, GL_RGBA, GL_FLOAT, (void*)0);
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
		
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		SwapBuffers(hdc);

		/*
			Check to see if there is a sound to play this frame.
		*/
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

				sys->input->controllers[i].up = UpdateButton((pad->wButtons & XINPUT_GAMEPAD_DPAD_UP), sys->input->controllers[i].up);
				sys->input->controllers[i].down = UpdateButton((pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN), sys->input->controllers[i].down);
				sys->input->controllers[i].left = UpdateButton((pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT), sys->input->controllers[i].left);
				sys->input->controllers[i].right = UpdateButton((pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT), sys->input->controllers[i].right);
				sys->input->controllers[i].start = UpdateButton((pad->wButtons & XINPUT_GAMEPAD_START), sys->input->controllers[i].start);
				sys->input->controllers[i].back = UpdateButton((pad->wButtons & XINPUT_GAMEPAD_BACK), sys->input->controllers[i].back);
				sys->input->controllers[i].lThumb = UpdateButton((pad->wButtons & XINPUT_GAMEPAD_LEFT_THUMB), sys->input->controllers[i].lThumb);
				sys->input->controllers[i].rThumb = UpdateButton((pad->wButtons & XINPUT_GAMEPAD_RIGHT_THUMB), sys->input->controllers[i].rThumb);
				sys->input->controllers[i].lShoulder = UpdateButton((pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER), sys->input->controllers[i].lShoulder);
				sys->input->controllers[i].rShoulder = UpdateButton((pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER), sys->input->controllers[i].rShoulder);
				sys->input->controllers[i].aButton = UpdateButton((pad->wButtons & XINPUT_GAMEPAD_A), sys->input->controllers[i].aButton);
				sys->input->controllers[i].bButton = UpdateButton((pad->wButtons & XINPUT_GAMEPAD_B), sys->input->controllers[i].bButton);
				sys->input->controllers[i].xButton = UpdateButton((pad->wButtons & XINPUT_GAMEPAD_X), sys->input->controllers[i].xButton);
				sys->input->controllers[i].yButton = UpdateButton((pad->wButtons & XINPUT_GAMEPAD_Y), sys->input->controllers[i].yButton);

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
			/*
				Setup for OpenGL.
			*/
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
			glClearColor(0.5f, 0.5f, 0.5f, 1.0f);

			char *vertShader, *fragShader;
			LoadTextFile("..\\data\\basic.vert", vertShader);
			LoadTextFile("..\\data\\basic.frag", fragShader);
			program = CreateBasicShader(vertShader, fragShader);
			
			delete[] vertShader;
			delete[] fragShader;

			glUseProgram(program);

			texLoc = glGetUniformLocation(program, "tex");

			glGenVertexArrays(1, &vertexArray);
			glBindVertexArray(vertexArray);

			glGenBuffers(2, pbo);
			glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo[1]);
			glBufferData(GL_PIXEL_UNPACK_BUFFER, sizeof(float) * windowWidth * windowHeight * 4, nullptr, GL_STREAM_DRAW);
			glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo[0]);
			glBufferData(GL_PIXEL_UNPACK_BUFFER, sizeof(float) * windowWidth * windowHeight * 4, nullptr, GL_STREAM_DRAW);

			glUniform1i(texLoc, 0);
			glActiveTexture(GL_TEXTURE0);
			glGenTextures(1, &texture);
			glBindTexture(GL_TEXTURE_2D, texture);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

			glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, windowWidth, windowHeight);
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, windowWidth, windowHeight, GL_RGBA, GL_FLOAT, (void*)0);

			glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

			/*
				Setup for XAudio2
			*/
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

