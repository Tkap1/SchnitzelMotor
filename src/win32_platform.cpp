
#pragma comment(lib, "Ole32.lib")

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <xaudio2.h>
#include "wglext.h"

// #############################################################################
//                           Platform Structs
// #############################################################################
struct Voice : IXAudio2VoiceCallback
{
	IXAudio2SourceVoice* voice;

	volatile int playing;

	void OnStreamEnd()
	{
		voice->Stop();
		InterlockedExchange((LONG*)&playing, false);
	}

	void OnBufferStart(void * pBufferContext)
	{
		InterlockedExchange((LONG*)&playing, true);
	}

	#pragma warning(push, 0)
	void OnVoiceProcessingPassEnd() { }
	void OnVoiceProcessingPassStart(UINT32 SamplesRequired) {  }
	void OnBufferEnd(void * pBufferContext) {  }
	void OnLoopEnd(void * pBufferContext) {  }
	void OnVoiceError(void * pBufferContext, HRESULT Error) { }
	#pragma warning(pop)
};

// #############################################################################
//                           Windows Globals
// #############################################################################
static HWND window;
static HDC dc;
static PFNGLDEBUGMESSAGECALLBACKPROC glDebugMessageCallback_ptr;
static Voice voiceArr[MAX_CONCURRENT_SOUNDS];

// #############################################################################
//                           Platform Implementations
// #############################################################################
LRESULT CALLBACK windows_window_callback(HWND window, UINT msg,
                                         WPARAM wParam, LPARAM lParam)
{
  LRESULT result = 0;

  switch(msg)
  {
    case WM_CLOSE:
    {
      running = false;

      break;
    }

    case WM_SIZE:
    {
      RECT rect = {0};
      GetClientRect(window, &rect);

      input->screenSize.x = (rect.right - rect.left);
      input->screenSize.y = (rect.bottom - rect.top);

      break;
    }

    case WM_KEYDOWN:
    case WM_KEYUP:
    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
    {
      bool isDown = (msg == WM_KEYDOWN) || (msg == WM_SYSKEYDOWN);
      KeyCodes keyCode = KeyCodeLookupTable[wParam];

      Key* key = &input->keys[keyCode];

      key->justPressed = !key->justPressed && !key->isDown && isDown;
      key->justReleased = !key->justReleased && key->isDown && !isDown;
      key->isDown = isDown;
      key->halfTransitionCount++;

      break;
    }

    default:
    {
      // Let windows handle the default input for now
      result = DefWindowProcA(window, msg, wParam, lParam);
    }
  }

  return result;
}

void platform_fill_keycode_lookup_table()
{
  KeyCodeLookupTable[VK_LBUTTON] = KEY_LEFT_MOUSE;
  KeyCodeLookupTable[VK_MBUTTON] = KEY_MIDDLE_MOUSE;
  KeyCodeLookupTable[VK_RBUTTON] = KEY_RIGHT_MOUSE;

  KeyCodeLookupTable['A'] = KEY_A;
  KeyCodeLookupTable['B'] = KEY_B;
  KeyCodeLookupTable['C'] = KEY_C;
  KeyCodeLookupTable['D'] = KEY_D;
  KeyCodeLookupTable['E'] = KEY_E;
  KeyCodeLookupTable['F'] = KEY_F;
  KeyCodeLookupTable['G'] = KEY_G;
  KeyCodeLookupTable['H'] = KEY_H;
  KeyCodeLookupTable['I'] = KEY_I;
  KeyCodeLookupTable['J'] = KEY_J;
  KeyCodeLookupTable['K'] = KEY_K;
  KeyCodeLookupTable['L'] = KEY_L;
  KeyCodeLookupTable['M'] = KEY_M;
  KeyCodeLookupTable['N'] = KEY_N;
  KeyCodeLookupTable['O'] = KEY_O;
  KeyCodeLookupTable['P'] = KEY_P;
  KeyCodeLookupTable['Q'] = KEY_Q;
  KeyCodeLookupTable['R'] = KEY_R;
  KeyCodeLookupTable['S'] = KEY_S;
  KeyCodeLookupTable['T'] = KEY_T;
  KeyCodeLookupTable['U'] = KEY_U;
  KeyCodeLookupTable['V'] = KEY_V;
  KeyCodeLookupTable['W'] = KEY_W;
  KeyCodeLookupTable['X'] = KEY_X;
  KeyCodeLookupTable['Y'] = KEY_Y;
  KeyCodeLookupTable['Z'] = KEY_Z;
  KeyCodeLookupTable['0'] = KEY_0;
  KeyCodeLookupTable['1'] = KEY_1;
  KeyCodeLookupTable['2'] = KEY_2;
  KeyCodeLookupTable['3'] = KEY_3;
  KeyCodeLookupTable['4'] = KEY_4;
  KeyCodeLookupTable['5'] = KEY_5;
  KeyCodeLookupTable['6'] = KEY_6;
  KeyCodeLookupTable['7'] = KEY_7;
  KeyCodeLookupTable['8'] = KEY_8;
  KeyCodeLookupTable['9'] = KEY_9;

  KeyCodeLookupTable[VK_SPACE] = KEY_SPACE,
  KeyCodeLookupTable[VK_OEM_3] = KEY_TICK,
  KeyCodeLookupTable[VK_OEM_MINUS] = KEY_MINUS,
  // TODO ???
  KeyCodeLookupTable[VK_OEM_PLUS] = KEY_EQUAL,
  KeyCodeLookupTable[VK_OEM_4] = KEY_LEFT_BRACKET,
  KeyCodeLookupTable[VK_OEM_6] = KEY_RIGHT_BRACKET,
  KeyCodeLookupTable[VK_OEM_1] = KEY_SEMICOLON,
  KeyCodeLookupTable[VK_OEM_7] = KEY_QUOTE,
  KeyCodeLookupTable[VK_OEM_COMMA] = KEY_COMMA,
  KeyCodeLookupTable[VK_OEM_PERIOD] = KEY_PERIOD,
  KeyCodeLookupTable[VK_OEM_2] = KEY_FORWARD_SLASH,
  KeyCodeLookupTable[VK_OEM_5] = KEY_BACKWARD_SLASH,
  KeyCodeLookupTable[VK_TAB] = KEY_TAB,
  KeyCodeLookupTable[VK_ESCAPE] = KEY_ESCAPE,
  KeyCodeLookupTable[VK_PAUSE] = KEY_PAUSE,
  KeyCodeLookupTable[VK_UP] = KEY_UP,
  KeyCodeLookupTable[VK_DOWN] = KEY_DOWN,
  KeyCodeLookupTable[VK_LEFT] = KEY_LEFT,
  KeyCodeLookupTable[VK_RIGHT] = KEY_RIGHT,
  KeyCodeLookupTable[VK_BACK] = KEY_BACKSPACE,
  KeyCodeLookupTable[VK_RETURN] = KEY_RETURN,
  KeyCodeLookupTable[VK_DELETE] = KEY_DELETE,
  KeyCodeLookupTable[VK_INSERT] = KEY_INSERT,
  KeyCodeLookupTable[VK_HOME] = KEY_HOME,
  KeyCodeLookupTable[VK_END] = KEY_END,
  KeyCodeLookupTable[VK_PRIOR] = KEY_PAGE_UP,
  KeyCodeLookupTable[VK_NEXT] = KEY_PAGE_DOWN,
  KeyCodeLookupTable[VK_CAPITAL] = KEY_CAPS_LOCK,
  KeyCodeLookupTable[VK_NUMLOCK] = KEY_NUM_LOCK,
  KeyCodeLookupTable[VK_SCROLL] = KEY_SCROLL_LOCK,
  KeyCodeLookupTable[VK_APPS] = KEY_MENU,

  KeyCodeLookupTable[VK_SHIFT] = KEY_SHIFT,
  KeyCodeLookupTable[VK_LSHIFT] = KEY_SHIFT,
  KeyCodeLookupTable[VK_RSHIFT] = KEY_SHIFT,

  KeyCodeLookupTable[VK_CONTROL] = KEY_CONTROL,
  KeyCodeLookupTable[VK_LCONTROL] = KEY_CONTROL,
  KeyCodeLookupTable[VK_RCONTROL] = KEY_CONTROL,

  KeyCodeLookupTable[VK_MENU] = KEY_ALT,
  KeyCodeLookupTable[VK_LMENU] = KEY_ALT,
  KeyCodeLookupTable[VK_RMENU] = KEY_ALT,

  KeyCodeLookupTable[VK_F1] = KEY_F1;
  KeyCodeLookupTable[VK_F2] = KEY_F2;
  KeyCodeLookupTable[VK_F3] = KEY_F3;
  KeyCodeLookupTable[VK_F4] = KEY_F4;
  KeyCodeLookupTable[VK_F5] = KEY_F5;
  KeyCodeLookupTable[VK_F6] = KEY_F6;
  KeyCodeLookupTable[VK_F7] = KEY_F7;
  KeyCodeLookupTable[VK_F8] = KEY_F8;
  KeyCodeLookupTable[VK_F9] = KEY_F9;
  KeyCodeLookupTable[VK_F10] = KEY_F10;
  KeyCodeLookupTable[VK_F11] = KEY_F11;
  KeyCodeLookupTable[VK_F12] = KEY_F12;

  KeyCodeLookupTable[VK_NUMPAD0] = KEY_NUMPAD_0;
  KeyCodeLookupTable[VK_NUMPAD1] = KEY_NUMPAD_1;
  KeyCodeLookupTable[VK_NUMPAD2] = KEY_NUMPAD_2;
  KeyCodeLookupTable[VK_NUMPAD3] = KEY_NUMPAD_3;
  KeyCodeLookupTable[VK_NUMPAD4] = KEY_NUMPAD_4;
  KeyCodeLookupTable[VK_NUMPAD5] = KEY_NUMPAD_5;
  KeyCodeLookupTable[VK_NUMPAD6] = KEY_NUMPAD_6;
  KeyCodeLookupTable[VK_NUMPAD7] = KEY_NUMPAD_7;
  KeyCodeLookupTable[VK_NUMPAD8] = KEY_NUMPAD_8;
  KeyCodeLookupTable[VK_NUMPAD9] = KEY_NUMPAD_9;
}

bool platform_create_window(int width, int height, char* title)
{
  HINSTANCE instance = GetModuleHandleA(0);

  // Setup and register window class
  HICON icon = LoadIcon(instance, IDI_APPLICATION);
  WNDCLASSA wc = {};
  wc.hInstance = instance;
  wc.hIcon = icon;
  wc.hCursor = LoadCursor(NULL, IDC_ARROW);
  wc.lpszClassName = title;
  wc.lpfnWndProc = windows_window_callback;  // Callback for Input into the window

  if (!RegisterClassA(&wc))
  {
    return false;
  }

  // Ex Style
  int exStyle = WS_EX_APPWINDOW;

  // Dw Style (window Styles),
  // (WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX)
  int dwStyle = WS_OVERLAPPEDWINDOW;

  PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB;
  PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB;

  // Windows specific OpenGL function loading
  {
    window = CreateWindowExA(exStyle,
                            title,       // Class Name, reference window class
                            title,       // acutal Title
                            dwStyle,
                            100,         // x - Pos
                            100,         // y - Pos
                            width,       // width
                            height,      // height
                            NULL,     // parent
                            NULL,        // menu
                            instance,
                            NULL);       // lpParam

    if(window == NULL)
    {
      SM_ASSERT(0, "Failed to create Windows Window");
      return false;
    }

    HDC fakeDC = GetDC(window);
    if(!fakeDC)
    {
      SM_ASSERT(0, "Failed to get HDC");
      return false;
    }

    PIXELFORMATDESCRIPTOR pfd = {0};
    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    pfd.cAlphaBits = 8;
    pfd.cDepthBits = 24;

    int pixelFormat = ChoosePixelFormat(fakeDC, &pfd);
    if(!pixelFormat)
    {
      SM_ASSERT(0, "Failed to ChoosePixelFormat");
      return false;
    }

    if(!SetPixelFormat(fakeDC, pixelFormat, &pfd))
    {
      SM_ASSERT(0, "Failed to SetPixelFormat");
      return false;
    }

    // Create OpenGL Render Context
    HGLRC fakeRC = wglCreateContext(fakeDC);
    if(!fakeRC)
    {
      SM_ASSERT(0, "Failed to wglCreateContext");
      return false;
    }

    if(!wglMakeCurrent(fakeDC, fakeRC))
    {
      SM_ASSERT(0, "Failed to wglMakeCurrent");
      return false;
    }

    wglChoosePixelFormatARB = (PFNWGLCHOOSEPIXELFORMATARBPROC)platform_load_gl_func("wglChoosePixelFormatARB");
    wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC)platform_load_gl_func("wglCreateContextAttribsARB");
    glDebugMessageCallback_ptr = (PFNGLDEBUGMESSAGECALLBACKPROC)platform_load_gl_func("glDebugMessageCallback");

    // Clean up the fake stuff
    wglMakeCurrent(fakeDC, 0);
    wglDeleteContext(fakeRC);
    ReleaseDC(window, fakeDC);

    // Can't add reuse the same (Device)Context, we already SetPixelFormat, etc.
    DestroyWindow(window);
  }

  // Setup window and initialize Windows specific OpenGL
  {
    window = CreateWindowExA(exStyle,
                            title,       // Class Name, reference window class
                            title,       // acutal Title
                            dwStyle,
                            100,         // x - Pos
                            100,         // y - Pos
                            width,       // width
                            height,      // height
                            NULL,        // parent
                            NULL,        // menu
                            instance,
                            NULL);       // lpParam

    if(window == NULL)
    {
      SM_ASSERT(0, "Failed to create Windows Window");
      return false;
    }

    // We need this globally, because we will need to call SwapBuffers(DC)!
    dc = GetDC(window);
    if(!dc)
    {
      SM_ASSERT(0, "Failed to get HDC");
      return false;
    }

    const int pixelAttribs[] =
    {
      WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
      WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
      WGL_DOUBLE_BUFFER_ARB,  GL_TRUE,
      WGL_SWAP_METHOD_ARB,    WGL_SWAP_COPY_ARB,
      WGL_PIXEL_TYPE_ARB,     WGL_TYPE_RGBA_ARB,
      WGL_ACCELERATION_ARB,   WGL_FULL_ACCELERATION_ARB,
      WGL_COLOR_BITS_ARB,     32,
      WGL_ALPHA_BITS_ARB,     8,
      WGL_DEPTH_BITS_ARB,     24,
      0 // Terminate with 0, otherwise OpenGL will throw an Error!
    };

    UINT numPixelFormats;
    int pixelFormat = 0;
    if(!wglChoosePixelFormatARB(dc, pixelAttribs,
                                0, // Float List
                                1, // Max Formats
                                &pixelFormat,
                                &numPixelFormats))
    {
      SM_ASSERT(0, "Failed to wglChoosePixelFormatARB");
      return false;
    }

    PIXELFORMATDESCRIPTOR pfd = {0};
    DescribePixelFormat(dc, pixelFormat, sizeof(PIXELFORMATDESCRIPTOR), &pfd);

    if(!SetPixelFormat(dc, pixelFormat, &pfd))
    {
      SM_ASSERT(0, "Failed to SetPixelFormat");
      return true;
    }

    const int contextAttribs[] =
    {
      WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
      WGL_CONTEXT_MINOR_VERSION_ARB, 3,
      WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
      0 // Terminate the Array
    };

    HGLRC rc = wglCreateContextAttribsARB(dc, 0, contextAttribs);
    if(!rc)
    {
      SM_ASSERT(0, "Failed to crate Render Context for OpenGL");
      return false;
    }

    if(!wglMakeCurrent(dc, rc))
    {
      SM_ASSERT(0, "Faield to wglMakeCurrent");
      return false;
    }

    // @Note(tkap, 13/07/2023): Load and call the vsync function. Your (cakez77) driver must be forcing vsync on all the time,
    // because you are not setting anywhere. This makes the game unplayable for everyone who doesn't have forced vsync. This fixes it.
    ((PFNWGLSWAPINTERVALEXTPROC)platform_load_gl_func("wglSwapIntervalEXT"))(1);
  }

  ShowWindow(window, SW_SHOW);

  return true;
}

void platform_update_window()
{
  MSG msg;

  while(PeekMessageA(&msg, window, 0, 0, PM_REMOVE))
  {
    TranslateMessage(&msg);
    DispatchMessage(&msg); // Calls the callback specified when creating the window
  }
}

void* platform_load_gl_func(char* funName)
{
  PROC proc = wglGetProcAddress(funName);
  if(!proc)
  {
    SM_ASSERT(0, "Failed to load OpenGL Function: %s", funName);
  }

  return (void*)proc;
}

void platform_swap_buffers()
{
  SwapBuffers(dc);
}

void glDebugMessageCallback (GLDEBUGPROC callback, const void *userParam)
{
  glDebugMessageCallback_ptr(callback, userParam);
}

void platform_reaload_dynamic_library()
{
  static HMODULE gameDLL;
  static long long lastTimestampGameDLL;

  long long currentTimestampGameDLL = get_timestamp("game.dll");
  if(currentTimestampGameDLL > lastTimestampGameDLL)
  {
    if(gameDLL)
    {
      BOOL freeResult = FreeLibrary(gameDLL);
      SM_ASSERT(freeResult, "Failed to FreeLibrary game.dll");
      gameDLL = nullptr;
      SM_TRACE("Freed game.dll");
    }

    while(!CopyFileA("game.dll", "game_load.dll", false)) { Sleep(10); }
    SM_TRACE("Copied game.dll");

    gameDLL = LoadLibraryA("game_load.dll");
    SM_ASSERT(gameDLL, "Failed to load game.dll");

    update_game_ptr = (update_game_type*)GetProcAddress(gameDLL, "update_game");
    SM_ASSERT(update_game_ptr, "Failed to load update_game function");
    lastTimestampGameDLL = currentTimestampGameDLL;
  }
}


bool thread_safe_set_bool_to_true(volatile int* var)
{
	return InterlockedCompareExchange((LONG*)var, true, false) == false;
}

bool play_sound(Sound sound)
{
	SM_ASSERT(sound.sampleCount > 0, "Sound has no samples");
	SM_ASSERT(sound.samples, "Sound has no samples");

	XAUDIO2_BUFFER buffer = {};
	buffer.Flags = XAUDIO2_END_OF_STREAM;
	buffer.AudioBytes = sound.sampleCount * NUM_CHANNELS * sizeof(short);
	buffer.pAudioData = (BYTE*)sound.samples;

	Voice* currVoice = nullptr;
	for(int voiceIdx = 0; voiceIdx < MAX_CONCURRENT_SOUNDS; voiceIdx++)
	{
		Voice* voice = &voiceArr[voiceIdx];
		if(!voice->playing)
		{
			if(thread_safe_set_bool_to_true(&voice->playing))
			{
				currVoice = voice;
				break;
			}
		}
	}

	if(currVoice == nullptr) { return false; }

	HRESULT hr = currVoice->voice->SubmitSourceBuffer(&buffer);
	if(FAILED(hr)) { return false; }

	currVoice->voice->Start();

	return true;
}

bool platform_init_sound()
{
	HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
	if(FAILED(hr)) { return false; }

	IXAudio2* xaudio2 = nullptr;
	hr = XAudio2Create(&xaudio2, 0, XAUDIO2_DEFAULT_PROCESSOR);
	if(FAILED(hr)) { return false; }

	IXAudio2MasteringVoice* master_voice = nullptr;
	hr = xaudio2->CreateMasteringVoice(&master_voice);
	if(FAILED(hr)) { return false; }

	WAVEFORMATEX wave = {};
	wave.wFormatTag = WAVE_FORMAT_PCM;
	wave.nChannels = NUM_CHANNELS;
	wave.nSamplesPerSec = SAMPLE_RATE;
	wave.wBitsPerSample = 16;
	wave.nBlockAlign = (NUM_CHANNELS * wave.wBitsPerSample) / 8;
	wave.nAvgBytesPerSec = SAMPLE_RATE * wave.nBlockAlign;

	for(int voiceIdx = 0; voiceIdx < MAX_CONCURRENT_SOUNDS; voiceIdx++)
	{
		Voice* voice = &voiceArr[voiceIdx];
		hr = xaudio2->CreateSourceVoice(&voice->voice, &wave, 0, XAUDIO2_DEFAULT_FREQ_RATIO, voice, nullptr, nullptr);
		voice->voice->SetVolume(0.25f);
		if(FAILED(hr)) { return false; }
	}

	return true;

}