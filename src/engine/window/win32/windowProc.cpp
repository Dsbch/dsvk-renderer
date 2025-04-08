#include <pch.h>
#include "window.h"

bool engine::winApiWindow::handleMouseEvent(engine::winApiWindow* winApiInst, HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	auto keyCode = engine::fromWinApiMouse(message);

	POINT cursorPos;
	cursorPos.x = GET_X_LPARAM(lParam);
	cursorPos.y = GET_Y_LPARAM(lParam);

	switch (message)
	{
	//case WM_INPUT:
	//{
	//	UINT dataSize;
	//	GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, NULL, &dataSize, sizeof(RAWINPUTHEADER)); //Need to populate data size first
	//	std::cout << GET_RAWINPUT_CODE_WPARAM(wParam) << " code thing\n";
	//	if (dataSize > 0)
	//	{
	//		std::vector<BYTE> rawdata(dataSize);

	//		if (GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, rawdata.data(), &dataSize, sizeof(RAWINPUTHEADER)) == dataSize)
	//		{
	//			RAWINPUT* raw = reinterpret_cast<RAWINPUT*>(rawdata.data());
	//			if (raw->header.dwType == RIM_TYPEMOUSE)
	//			{
	//			}
	//		}
	//	}

	//	return true;
	//}
	case WM_LBUTTONUP:
	{
		winApiInst->mKeyUp[keyCode] = { keyCode, { cursorPos.x, cursorPos.y } };
		return true;
	}
	case WM_MBUTTONUP:
	{
		winApiInst->mKeyUp[keyCode] = { keyCode, { cursorPos.x, cursorPos.y } };
		return true;
	}
	case WM_RBUTTONUP:
	{
		winApiInst->mKeyUp[keyCode] = { keyCode, { cursorPos.x, cursorPos.y } };
		return true;
	}
	case WM_LBUTTONDOWN:
	{
		winApiInst->mKeyDown[keyCode] = { keyCode, { cursorPos.x, cursorPos.y } };
		return true;
	}
	case WM_MBUTTONDOWN:
	{
		winApiInst->mKeyDown[keyCode] = { keyCode, { cursorPos.x, cursorPos.y } };
		return true;
	}
	case WM_RBUTTONDOWN:
	{
		winApiInst->mKeyDown[keyCode] = { keyCode, { cursorPos.x, cursorPos.y } };
		return true;
	}
	}

	return false;
}

bool engine::winApiWindow::handleKeyboardEvent(engine::winApiWindow* winApiInst, HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	auto keyCode = engine::fromWinApiKey(wParam);

	switch (message)
	{
	case WM_KEYUP:
	{
		POINT cursorPos;
		GetCursorPos(&cursorPos);

		winApiInst->mKeyUp[keyCode] = { keyCode, { cursorPos.x, cursorPos.y } };
		return true;
	}
	case WM_KEYDOWN:
	{
		POINT cursorPos;
		GetCursorPos(&cursorPos);

		winApiInst->mKeyDown[keyCode] = { keyCode, { cursorPos.x, cursorPos.y } };
		return true;
	}
	}

	return false;
}

bool engine::winApiWindow::handleCloseEvent(engine::winApiWindow* winApiInst, HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (message == WM_DESTROY)
	{
		PostQuitMessage(0);
		winApiInst->mCtx.getDispatcher()->dispatch(engine::closeEvent{});
		return true;
	}

	return false;
}

bool engine::winApiWindow::handleResizeEvent(engine::winApiWindow* winApiInst, HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (message == WM_SIZE)
	{
		winApiInst->mCtx.getDispatcher()->dispatch(engine::windowResizeEvent{ LOWORD(lParam), HIWORD(lParam) });
		return true;
	}

	return false;
}

bool engine::winApiWindow::handlePaintEvent(engine::winApiWindow* winApiInst, HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (message == WM_PAINT)
		return true;

	return false;
}


LRESULT engine::winApiWindow::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	PROFILE_FUNC();

	hwndTableMu.lock();
	auto pThis = hwndTable.find(hWnd);
	auto end = hwndTable.end();
	hwndTableMu.unlock();

	if (pThis != end)
	{
		bool handled = false;

		handled |= handleKeyboardEvent(pThis->second, hWnd, message, wParam, lParam);
		handled |= handleMouseEvent(pThis->second, hWnd, message, wParam, lParam);
		handled |= handleResizeEvent(pThis->second, hWnd, message, wParam, lParam);
		handled |= handlePaintEvent(pThis->second, hWnd, message, wParam, lParam);
		handled |= handleCloseEvent(pThis->second, hWnd, message, wParam, lParam);

		if (handled)
			return 0;
	}

	return DefWindowProc(hWnd, message, wParam, lParam);
}
