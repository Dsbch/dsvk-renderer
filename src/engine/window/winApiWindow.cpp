#include <pch.h>
#include "winApiWindow.h"
#include <glad/glad.h>

core::error engine::winApiWindow::mCreateWndClassErr;
std::once_flag engine::winApiWindow::mIsWindowClassCreated;
WNDCLASSEX engine::winApiWindow::mWndClass;

static HMODULE getThisModuleHandle()
{
	//Returns module handle where this function is running in: EXE or DLL
	HMODULE hModule = NULL;

	GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
		GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
		(LPCTSTR)getThisModuleHandle, &hModule);

	return hModule;
}

LRESULT engine::winApiWindow::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	PROFILE_FUNC();

	switch (message)
	{
	case WM_PAINT:
		// Here your application is laid out.
		// For this introduction, we just do nothing.
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_SIZE: //Check if the window has been resized
	{
		int width = LOWORD(lParam);  // Extracts new width
		int height = HIWORD(lParam); // Extracts new height
	
		engine::eventDispatcher::dispatch<engine::windowResizeEvent>({ uint32_t(width), uint32_t(height) });

		break;
	}
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}

	return 0;
}

void engine::winApiWindow::createWindowClass(const std::string& applicationName)
{
	PROFILE_FUNC();

	auto appName = std::wstring(applicationName.begin(), applicationName.end());
	LPCWSTR lcpAppName = appName.c_str();

	auto hInstace = getThisModuleHandle();

	mWndClass.cbSize = sizeof(WNDCLASSEX); // size of structure 
	mWndClass.style = CS_HREDRAW |
		CS_VREDRAW;                    // redraw if size changes 
	mWndClass.lpfnWndProc = WndProc;     // points to window procedure 
	mWndClass.cbClsExtra = 0;                // no extra class memory 
	mWndClass.cbWndExtra = 0;                // no extra window memory 
	mWndClass.hInstance = hInstace;         // handle to instance 
	mWndClass.hIcon = LoadIcon(NULL,
		IDI_APPLICATION);              // predefined app. icon 
	mWndClass.hCursor = LoadCursor(NULL,
		IDC_ARROW);                    // predefined arrow 
	mWndClass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH); // white background brush 
	mWndClass.lpszMenuName = NULL;    // name of menu resource 
	mWndClass.lpszClassName = lcpAppName;   // name of window class 
	mWndClass.hIconSm = (HICON)LoadImage(hInstace, // small class icon 
		MAKEINTRESOURCE(5),
		IMAGE_ICON,
		GetSystemMetrics(SM_CXSMICON),
		GetSystemMetrics(SM_CYSMICON),
		LR_DEFAULTCOLOR);

	if (!RegisterClassEx(&mWndClass))
	{
		mCreateWndClassErr = std::move(core::error("error in RegisterClassEx"));

		return;
	}
}

engine::winApiWindow::winApiWindow(const std::string& name, std::uint32_t width, std::uint32_t heigth, bool isFullscreen, const std::string& applicationName, int noCmdShow)
	: window(name, width, heigth, isFullscreen), mApplicationName(applicationName), mNoCmdShow(noCmdShow), mHWnd(), mHdc(), mHrc()
{
	PROFILE_FUNC();

	std::call_once(mIsWindowClassCreated, createWindowClass, mApplicationName);
	if (mCreateWndClassErr)
	{
		mErr = mCreateWndClassErr;
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
		isFullscreen ? WS_POPUP | WS_VISIBLE : WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT,
		mWidth, mHeight,
		NULL,
		NULL,
		hInstace,
		NULL
	);

	if (!mHWnd)
	{
		mErr = std::move(core::error("error in CreateWindowEx"));

		return;
	}

	ShowCursor(!isFullscreen);

	// The parameters to ShowWindow explained:
	// hWnd: the value returned from CreateWindow
	// nCmdShow: the fourth parameter from WinMain
	ShowWindow(mHWnd, mNoCmdShow);
	UpdateWindow(mHWnd);
}

engine::winApiWindow::~winApiWindow()
{
	if (mHrc)                                            // Do We Have A Rendering Context?
	{
		if (!wglMakeCurrent(NULL, NULL))                 // Are We Able To Release The DC And RC Contexts?
		{
			LOGERROR("can't release opengl context");
		}

		if (!wglDeleteContext(mHrc))                     // Are We Able To Delete The RC?
		{
			LOGERROR("can't release rendering context");
		}
	}

	auto appName = std::wstring(mApplicationName.begin(), mApplicationName.end());
	LPCWSTR lpcAppName = appName.c_str();

	UnregisterClass(lpcAppName, getThisModuleHandle());
}

engine::winApiWindow::winApiWindow(const winApiWindow& other)
	: window(other)
{
	this->mApplicationName = other.mApplicationName;
	this->mHWnd = other.mHWnd;
	this->mNoCmdShow = other.mNoCmdShow;
	this->mHdc = other.mHdc;
	this->mHrc = other.mHrc;
}

engine::winApiWindow::winApiWindow(winApiWindow&& other) noexcept
	: window(std::move(other)), mApplicationName(std::move(other.mApplicationName)), mHWnd(std::move(other.mHWnd)), mNoCmdShow(std::move(other.mNoCmdShow)), mHdc(std::move(other.mHdc)), mHrc(std::move(other.mHrc))
{
}

engine::winApiWindow& engine::winApiWindow::operator=(const winApiWindow& other)
{
	if (this != &other)
	{
		engine::winApiWindow tmp(other);

		this->mApplicationName.swap(tmp.mApplicationName);
		this->mHWnd = other.mHWnd;
		this->mNoCmdShow = other.mNoCmdShow;
		this->mHdc = other.mHdc;
		this->mHrc = other.mHrc;
	}

	return *this;
}

engine::winApiWindow& engine::winApiWindow::operator=(winApiWindow&& other) noexcept
{
	this->mApplicationName.swap(other.mApplicationName);
	this->mHWnd = std::move(other.mHWnd);
	this->mNoCmdShow = std::move(other.mNoCmdShow);
	this->mHdc = std::move(other.mHdc);
	this->mHrc = std::move(other.mHrc);

	return *this;
}

core::error engine::winApiWindow::makeOpenglContext()
{
	PIXELFORMATDESCRIPTOR pfd =              // pfd Tells Windows How We Want Things To Be
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
		return { "error on GetDC call" };                               // Return false
	}

	int pixelFormat = 0;

	if (!(pixelFormat=ChoosePixelFormat(mHdc, &pfd))) // Did Windows Find A Matching Pixel Format?
	{
		return { "error on ChoosePixelFormat call" };                               // Return false
	}

	if (!SetPixelFormat(mHdc, pixelFormat, &pfd))       // Are We Able To Set The Pixel Format?
	{
		return { "error on SetPixelFormat call" };                               // Return false
	}

	if (!(mHrc = wglCreateContext(mHdc)))               // Are We Able To Get A Rendering Context?
	{
		return { "error on wglCreateContext call" };                               // Return false
	}

	if (!wglMakeCurrent(mHdc, mHrc))                    // Try To Activate The Rendering Context
	{
		return { "error on wglMakeCurrent call" };                               // Return false
	}

	return {};
}

void engine::winApiWindow::updateWindowState()
{
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0))
	{
		// move to render.
		// Clear screen to red.
		glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		// end move to renderer.

		TranslateMessage(&msg);
		DispatchMessage(&msg);
		SwapBuffers(mHdc);
	}

	return;
}
