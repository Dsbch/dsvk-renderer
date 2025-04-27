#include <pch.h>
#include "window.h"

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

		auto appName = std::wstring(mApplicationName.begin(), mApplicationName.end());
		LPCWSTR lpcAppName = appName.c_str();

		auto title = std::wstring(mName.begin(), mName.end());
		LPCWSTR lpcTitle = title.c_str();

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
		mHWnd = CreateWindowEx(
			WS_EX_APPWINDOW,
			lpcAppName,
			lpcTitle,
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

	winApiWindow::winApiWindow(context ctx, const std::string& name, std::uint32_t width, std::uint32_t heigth, bool isFullscreen, const std::string& applicationName, bool showCursor)
		: baseWindow(ctx, name, width, heigth, isFullscreen, showCursor), mApplicationName(applicationName), mHWnd(), mHdc(), mHrc()
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
			if (!wglMakeCurrent(NULL, NULL))                 // Are We Able To Release The DC And RC Contexts?
			{
				logLastError("can't release opengl context");
			}

			if (!wglDeleteContext(mHrc))                     // Are We Able To Delete The RC?
			{
				logLastError("can't release rendering context");
			}
		}

		if (mHWnd)
		{
			PostMessage(mHWnd, WM_CLOSE, 0, 0);
		}

		mCtx.getDispatcher()->queueEvent(std::make_shared<closeEvent>());

		{
			std::lock_guard<std::mutex> l{ hwndTableMu };
			hwndTable.erase(mHWnd);
		}
	}

	engine::error winApiWindow::makeOpenglContext()
	{
		PIXELFORMATDESCRIPTOR pfd =						// pfd Tells Windows How We Want Things To Be
		{
			sizeof(PIXELFORMATDESCRIPTOR),              // Size Of This Pixel Format Descriptor
			1,                                          // Version Number
			PFD_DRAW_TO_WINDOW |                        // Format Must Support Window
			PFD_SUPPORT_OPENGL |                        // Format Must Support OpenGL
			PFD_DOUBLEBUFFER,                           // Must Support Double Buffering
			PFD_TYPE_RGBA,                              // Request An RGBA Format
			32,											// Select Our Color Depth
			0, 0, 0, 0, 0, 0,                           // Color Bits Ignored
			0,                                          // No Alpha Buffer
			0,                                          // Shift Bit Ignored
			0,                                          // No Accumulation Buffer
			0, 0, 0, 0,                                 // Accumulation Bits Ignored
			16,                                         // 16Bit Z-Buffer (Depth Buffer)  
			0,                                          // No Stencil Buffer
			0,                                          // No Auxiliary Buffer
			PFD_MAIN_PLANE,                             // Main Drawing Layer
			0,                                          // Reserved
			0, 0, 0                                     // Layer Masks Ignored
		};

		if (!(mHdc = GetDC(mHWnd)))                     // Did We Get A Device Context?
		{
			return { "error on GetDC call" };
		}

		int pixelFormat = 0;

		if (!(pixelFormat = ChoosePixelFormat(mHdc, &pfd))) // Did Windows Find A Matching Pixel Format?
		{
			return { "error on ChoosePixelFormat call" };
		}

		if (!SetPixelFormat(mHdc, pixelFormat, &pfd))       // Are We Able To Set The Pixel Format?
		{
			return { "error on SetPixelFormat call" };
		}

		if (!(mHrc = wglCreateContext(mHdc)))               // Are We Able To Get A Rendering Context?
		{
			return { "error on wglCreateContext call" };
		}

		if (!wglMakeCurrent(mHdc, mHrc))                    // Try To Activate The Rendering Context
		{
			return { "error on wglMakeCurrent call" };
		}

		return {};
	}

	void winApiWindow::pollInput()
	{
		std::lock_guard<std::mutex> l(mEvenetQueueMu);

		// Queue mouse move events and keyDown events.
		for (int i = 0; i < mEventQueue.size(); i++)
		{
			mCtx.getDispatcher()->queueEvent(mEventQueue.front());
			mEventQueue.pop();
		}

		// Queue still pressed keys.
		for (auto& [key, val] : mKeyDown)
		{
			mCtx.getDispatcher()->queueEvent(val);
		}
	}

	bool winApiWindow::isKeyPressed(key k)
	{
		std::lock_guard<std::mutex> l(mEvenetQueueMu);

		return mKeyDown.find(k) != mKeyDown.end();
	}

	void winApiWindow::startPolling()
	{
		MSG msg{};
		auto peekNotInput = [&]
			{
				if (GetForegroundWindow() != mHWnd)
				{
					return PeekMessage(&msg, NULL, 0, 0, PM_REMOVE);
				}

				auto ret = PeekMessage(&msg, NULL, 0, WM_INPUT - 1, PM_REMOVE);
				if (!ret)
				{
					ret = PeekMessage(&msg, NULL, WM_INPUT + 1, std::numeric_limits<UINT>::max(), PM_REMOVE);
				}

				return ret;
			};

		while (GetMessage(&msg, NULL, 0, 0) > 0)
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
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