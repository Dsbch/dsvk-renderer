#include <pch.h>
#include "window.h"

core::error engine::winApiWindow::createWndClassErr;
std::once_flag engine::winApiWindow::isWindowClassCreated;
WNDCLASSEX engine::winApiWindow::wndClass;
std::map<HWND, engine::winApiWindow*> engine::winApiWindow::hwndTable;
std::mutex engine::winApiWindow::hwndTableMu;

static HMODULE getThisModuleHandle()
{
	//Returns module handle where this function is running in: EXE or DLL
	HMODULE hModule = NULL;

	GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
		GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
		(LPCTSTR)getThisModuleHandle, &hModule);

	return hModule;
}

void engine::winApiWindow::createWindowClass(const std::string& applicationName)
{
	PROFILE_FUNC();

	if (!SetProcessDPIAware())
	{
		LOGERROR("error on call to SetProcessDPIAware");
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
		createWndClassErr = std::move(core::error("error in RegisterClassEx"));

		return;
	}
}

engine::winApiWindow::winApiWindow(engine::context ctx, const std::string& name, std::uint32_t width, std::uint32_t heigth, bool isFullscreen, const std::string& applicationName, bool showCursor)
	: baseWindow(ctx, name, width, heigth, isFullscreen, showCursor), mApplicationName(applicationName), mHWnd(), mHdc(), mHrc()
{
	PROFILE_FUNC();

	std::call_once(isWindowClassCreated, [this](const std::string& appName) { this->createWindowClass(appName); }, mApplicationName);
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
		mErr = std::move(core::error("error in CreateWindowEx"));

		return;
	}

	hwndTableMu.lock();
	hwndTable[mHWnd] = this;
	hwndTableMu.unlock();

	if (!mShowCursor)
		ShowCursor(mShowCursor);

	ShowWindow(mHWnd, SW_SHOW);
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

	hwndTable.erase(mHWnd);
}

engine::winApiWindow::winApiWindow(const winApiWindow& other)
	: baseWindow(other)
{
	this->mApplicationName = other.mApplicationName;
	this->mHWnd = other.mHWnd;
	this->mHdc = other.mHdc;
	this->mHrc = other.mHrc;
}

engine::winApiWindow& engine::winApiWindow::operator=(const winApiWindow& other)
{
	if (this != &other)
	{
		engine::winApiWindow tmp(other);

		this->mApplicationName.swap(tmp.mApplicationName);
		this->mHWnd = other.mHWnd;
		this->mHdc = other.mHdc;
		this->mHrc = other.mHrc;
	}

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
		return { "error on GetDC call" };                               
	}

	int pixelFormat = 0;

	if (!(pixelFormat=ChoosePixelFormat(mHdc, &pfd))) // Did Windows Find A Matching Pixel Format?
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

void engine::winApiWindow::updateWindowState()
{
	MSG msg;
	if (!PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		return;

	TranslateMessage(&msg);
	DispatchMessage(&msg);
}

void engine::winApiWindow::dispatchInput()
{
	for (auto crnt : mKeyUp)
	{
		mCtx.getDispatcher()->dispatch(crnt.second);
		mKeyDown.erase(crnt.first);
	}
	mKeyUp.clear();


	for (auto crnt : mKeyDown)
	{
		mCtx.getDispatcher()->dispatch(crnt.second);
	}
}

void engine::winApiWindow::swapBuffers() const
{
	SwapBuffers(mHdc);
}

core::error engine::winApiWindow::checkError()
{
	return mErr;
}

void engine::winApiWindow::toggleCursor()
{
	mShowCursor = !mShowCursor;
	ShowCursor(mShowCursor);
}
