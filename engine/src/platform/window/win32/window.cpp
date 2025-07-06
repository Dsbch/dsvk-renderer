#include <pch.h>
#include "window.h"

typedef HGLRC(WINAPI* PFNWGLCREATECONTEXTATTRIBSARBPROC)(HDC hDC, HGLRC hShareContext, const int* attribList);

#define WGL_CONTEXT_MAJOR_VERSION_ARB 0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB 0x2092
#define WGL_CONTEXT_FLAGS_ARB         0x2094
#define WGL_CONTEXT_PROFILE_MASK_ARB  0x9126

#define WGL_CONTEXT_DEBUG_BIT_ARB     0x0001
#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB 0x00000001

namespace engine
{
	engine::error winApiWindow::createWndClassErr;
	std::once_flag winApiWindow::isWindowClassCreated;
	WNDCLASSEX winApiWindow::wndClass;
	std::map<HWND, winApiWindow*> winApiWindow::hwndTable;
	std::mutex winApiWindow::hwndTableMu;

	void winApiWindow::logLastError(const std::string& prefix = "")
	{
#ifndef DEBUG
		return;
#endif // !DEBUG
		DWORD error = GetLastError();
		if (error == 0) {
			return;
		}

		LPVOID msgBuffer;
		FormatMessageA(
			FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			nullptr,
			error,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			reinterpret_cast<LPSTR>(&msgBuffer),
			0,
			nullptr
		);

		LOGERROR("{} {}", prefix, static_cast<LPCSTR>(msgBuffer));

		LocalFree(msgBuffer);
	}

	static HMODULE getThisModuleHandle()
	{
		//Returns module handle where this function is running in: EXE or DLL
		HMODULE hModule = NULL;

		GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
			GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
			(LPCTSTR)getThisModuleHandle, &hModule);

		return hModule;
	}

	void winApiWindow::createWindowClass(const std::string& applicationName)
	{
		PROFILE_FUNC();

		if (!SetProcessDPIAware())
		{
			logLastError("error on call to SetProcessDPIAware");
		}

		auto appName = std::wstring(applicationName.begin(), applicationName.end());
		LPCWSTR lcpAppName = appName.c_str();

		auto hInstace = getThisModuleHandle();

		wndClass.cbSize = sizeof(WNDCLASSEX); // size of structure 
		wndClass.style = CS_HREDRAW |
			CS_VREDRAW;                    // redraw if size changes 
		wndClass.lpfnWndProc = WndProc;     // points to window procedure 
		wndClass.cbClsExtra = 0;                // no extra class memory 
		wndClass.cbWndExtra = 0;                // no extra window memory 
		wndClass.hInstance = hInstace;         // handle to instance 
		wndClass.hIcon = LoadIcon(NULL,
			IDI_APPLICATION);              // predefined app. icon 
		wndClass.hCursor = LoadCursor(NULL,
			IDC_ARROW);                    // predefined arrow 
		wndClass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH); // white background brush 
		wndClass.lpszMenuName = NULL;    // name of menu resource 
		wndClass.lpszClassName = lcpAppName;   // name of window class 
		wndClass.hIconSm = (HICON)LoadImage(hInstace, // small class icon 
			MAKEINTRESOURCE(5),
			IMAGE_ICON,
			GetSystemMetrics(SM_CXSMICON),
			GetSystemMetrics(SM_CYSMICON),
			LR_DEFAULTCOLOR);

		if (!RegisterClassEx(&wndClass))
		{
			createWndClassErr = std::move(engine::error("error in RegisterClassEx"));

			return;
		}
	}

	void winApiWindow::registerInputDevices() const
	{
		RAWINPUTDEVICE rid[2];

		// Mouse
		rid[0].usUsagePage = HID_USAGE_PAGE_GENERIC;
		rid[0].usUsage = HID_USAGE_GENERIC_MOUSE;
		rid[0].dwFlags = RIDEV_INPUTSINK;
		rid[0].hwndTarget = mHWnd;

		// Keyboard
		rid[1].usUsagePage = HID_USAGE_PAGE_GENERIC;
		rid[1].usUsage = HID_USAGE_GENERIC_KEYBOARD;
		rid[1].dwFlags = RIDEV_NOLEGACY | RIDEV_INPUTSINK;
		rid[1].hwndTarget = mHWnd;

		if (!RegisterRawInputDevices(rid, 2, sizeof(RAWINPUTDEVICE)))
		{
			logLastError("can't register input devices");
		}
	}

	void winApiWindow::createWindow()
	{
		std::call_once(isWindowClassCreated, [&]()->void { this->createWindowClass(mApplicationName); });
		if (createWndClassErr)
		{
			mErr = createWndClassErr;
			return;
		}

		auto hInstace = getThisModuleHandle();

		// The parameters to CreateWindowEx explained:
		// WS_EX_OVERLAPPEDWINDOW : An optional extended window style.
		// szWindowClass: the name of the application
		// szTitle: the text that appears in the title bar
		// WS_OVERLAPPEDWINDOW: the type of window to create
		// CW_USEDEFAULT, CW_USEDEFAULT: initial position (x, y)
		// 500, 100: initial size (width, length)
		// NULL: the parent of this window
		// NULL: this application does not have a menu bar
		// hInstance: the first parameter from WinMain
		// NULL: not used in this application
		mHWnd = CreateWindowExA(
			WS_EX_APPWINDOW,
			mApplicationName.c_str(),
			mName.c_str(),
			mIsFullscreen ? WS_POPUP | WS_VISIBLE : WS_OVERLAPPEDWINDOW,
			CW_USEDEFAULT, CW_USEDEFAULT,
			mWidth, mHeight,  // Corrected sizes
			NULL,
			NULL,
			hInstace,
			NULL
		);

		if (!mHWnd)
		{
			mErr = std::move(engine::error("error in CreateWindowEx"));

			return;
		}

		{
			std::lock_guard<std::mutex> l{ hwndTableMu };
			hwndTable[mHWnd] = this;
		}

		if (!mShowCursor)
			ShowCursor(mShowCursor);

		ShowWindow(mHWnd, SW_SHOW);
		UpdateWindow(mHWnd);
	}

	winApiWindow::winApiWindow(std::shared_ptr<context> ctx, const std::string& name, std::uint32_t width, std::uint32_t heigth, bool isFullscreen, const std::string& applicationName, bool showCursor)
		: window(ctx, name, width, heigth, isFullscreen, showCursor), mApplicationName(applicationName), mHWnd(), mHdc(), mHrc()
	{
		PROFILE_FUNC();

		createWindow();
		registerInputDevices();

		if (!mShowCursor)
			ShowCursor(mShowCursor);

		ShowWindow(mHWnd, SW_SHOW);
		UpdateWindow(mHWnd);
	}

	winApiWindow::~winApiWindow()
	{
		if (mHrc)                                            // Do We Have A Rendering Context?
		{
#ifdef OPENGL
			if (!wglMakeCurrent(NULL, NULL))                 // Are We Able To Release The DC And RC Contexts?
			{
				logLastError("can't release opengl context");
			}

			if (!wglDeleteContext(mHrc))                     // Are We Able To Delete The RC?
			{
				logLastError("can't release rendering context");
			}
#endif // OPENGL
		}

		if (mHWnd)
		{
			if(!PostMessage(mHWnd, WM_CLOSE, 0, 0))
				logLastError("can't close window in destructor");
		}

		mCtx->mEventDispatcher->queueEvent(std::make_shared<closeEvent>());

		{
			std::lock_guard<std::mutex> l{ hwndTableMu };
			hwndTable.erase(mHWnd);
		}
	}

	engine::error winApiWindow::makeOpenglContext()
	{
#ifdef OPENGL
		PIXELFORMATDESCRIPTOR pfd =
		{
			sizeof(PIXELFORMATDESCRIPTOR),
			1,
			PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
			PFD_TYPE_RGBA,
			32,
			0,0,0,0,0,0,
			0,
			0,
			0,
			0,0,0,0,
			16,
			0,
			0,
			PFD_MAIN_PLANE,
			0,
			0,0,0
		};

		if (!(mHdc = GetDC(mHWnd)))
			return { "error on GetDC call" };

		int pixelFormat = ChoosePixelFormat(mHdc, &pfd);
		if (!pixelFormat)
			return { "error on ChoosePixelFormat call" };

		if (!SetPixelFormat(mHdc, pixelFormat, &pfd))
			return { "error on SetPixelFormat call" };

#ifdef DEBUG
		// Create temporary context to load wglCreateContextAttribsARB
		HGLRC tempContext = wglCreateContext(mHdc);
		if (!tempContext)
			return { "error on wglCreateContext creation (temp)" };

		if (!wglMakeCurrent(mHdc, tempContext))
			return { "error on wglMakeCurrent (temp context)" };

		// Load pointer to wglCreateContextAttribsARB
		auto wglCreateContextAttribsARB =
			(PFNWGLCREATECONTEXTATTRIBSARBPROC)wglGetProcAddress("wglCreateContextAttribsARB");
		if (!wglCreateContextAttribsARB)
		{
			wglMakeCurrent(nullptr, nullptr);
			wglDeleteContext(tempContext);
			return { "wglCreateContextAttribsARB not supported" };
		}

		// Attributes for OpenGL 4.6 core debug context
		int attribs[] =
		{
			WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
			WGL_CONTEXT_MINOR_VERSION_ARB, 6,
			WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_DEBUG_BIT_ARB,
			WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
			0
		};

		mHrc = wglCreateContextAttribsARB(mHdc, 0, attribs);

		// Delete temporary context and release it
		wglMakeCurrent(nullptr, nullptr);
		wglDeleteContext(tempContext);

		if (!mHrc)
			return { "Failed to create OpenGL debug context" };

		if (!wglMakeCurrent(mHdc, mHrc))
			return { "Failed to make debug context current" };

#else // DEBUG
		// Normal context creation
		mHrc = wglCreateContext(mHdc);
		if (!mHrc)
			return { "error on wglCreateContext call" };

		if (!wglMakeCurrent(mHdc, mHrc))
			return { "error on wglMakeCurrent call" };
#endif
#endif // OPENGL

		return {};
	}


	void winApiWindow::pollInput()
	{
		std::lock_guard<std::mutex> l(mEvenetQueueMu);

		// Queue mouse move events and keyDown events.
		for (int i = 0; i < mEventQueue.size(); i++)
		{
			mCtx->mEventDispatcher->queueEvent(mEventQueue.front());
			mEventQueue.pop();
		}

		// Queue still pressed keys.
		for (auto& [key, val] : mKeyDown)
		{
			mCtx->mEventDispatcher->queueEvent(val);
		}
	}

	bool winApiWindow::isKeyPressed(key k)
	{
		std::lock_guard<std::mutex> l(mEvenetQueueMu);

		return mKeyDown.find(k) != mKeyDown.end();
	}

	HWND winApiWindow::getHandle()
	{
		return mHWnd;
	}

	void winApiWindow::startPolling()
	{
		MSG msg{};
		while (GetMessage(&msg, NULL, 0, 0) > 0)
		{
			if (msg.message == WM_CLOSE || msg.message == WM_DESTROY || msg.message == WM_QUIT)
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
				continue;
			}

			if (GetForegroundWindow() == mHWnd)
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
	}

	void winApiWindow::swapBuffers() const
	{
		SwapBuffers(mHdc);
	}

	engine::error winApiWindow::checkError()
	{
		return mErr;
	}

	void winApiWindow::toggleCursor()
	{
		mShowCursor = !mShowCursor;
		ShowCursor(mShowCursor);
	}
}