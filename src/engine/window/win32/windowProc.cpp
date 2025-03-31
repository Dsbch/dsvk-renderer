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
	case WM_MOUSEMOVE:
		winApiInst->mDispatcher->template dispatch<engine::mouseMoveEvent>(
			{ {cursorPos.x, cursorPos.y } });
		return true;
	case WM_LBUTTONUP:
	{
		winApiInst->mDispatcher->template dispatch<engine::keyUpEvent>(
			{ keyCode, { cursorPos.x, cursorPos.y } });
		return true;
	}
	case WM_MBUTTONUP:
	{
		winApiInst->mDispatcher->template dispatch<engine::keyUpEvent>(
			{ keyCode, { cursorPos.x, cursorPos.y } });
		return true;
	}
	case WM_RBUTTONUP:
	{
		winApiInst->mDispatcher->template dispatch<engine::keyUpEvent>(
			{ keyCode, { cursorPos.x, cursorPos.y } });
		return true;
	}
	case WM_LBUTTONDOWN:
	{
		winApiInst->mDispatcher->template dispatch<engine::keyDownEvent>(
			{ keyCode, { cursorPos.x, cursorPos.y } });
		return true;
	}
	case WM_MBUTTONDOWN:
	{
		winApiInst->mDispatcher->template dispatch<engine::keyDownEvent>(
			{ keyCode, { cursorPos.x, cursorPos.y } });
		return true;
	}
	case WM_RBUTTONDOWN:
	{
		winApiInst->mDispatcher->template dispatch<engine::keyDownEvent>(
			{ keyCode, { cursorPos.x, cursorPos.y } });
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
		ScreenToClient(hWnd, &cursorPos);
		LOGINFO("{}, UP", int(keyCode));

		winApiInst->mDispatcher->template dispatch<engine::keyUpEvent>(
			{ keyCode, { cursorPos.x, cursorPos.y } });
		return true;
	}
	case WM_KEYDOWN:
	{
		POINT cursorPos;
		GetCursorPos(&cursorPos);
		ScreenToClient(hWnd, &cursorPos);

		LOGINFO("{}, down", int(keyCode));

		winApiInst->mDispatcher->template dispatch<engine::keyDownEvent>(
			{ keyCode, { cursorPos.x, cursorPos.y } });
		return true;
	}
	}

	return false;
}

bool engine::winApiWindow::handleCloseEvent(engine::winApiWindow* winApiInst, HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (message == WM_DESTROY)
	{
		winApiInst->mDispatcher->template dispatch<engine::closeEvent>({});
		PostQuitMessage(0);
		return true;
	}

	return false;
}

bool engine::winApiWindow::handleResizeEvent(engine::winApiWindow* winApiInst, HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (message == WM_SIZE)
	{
		winApiInst->mDispatcher->template dispatch<engine::windowResizeEvent>({ LOWORD(lParam), HIWORD(lParam) });
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
		handled |= handleCloseEvent(pThis->second, hWnd, message, wParam, lParam);
		handled |= handleResizeEvent(pThis->second, hWnd, message, wParam, lParam);
		handled |= handlePaintEvent(pThis->second, hWnd, message, wParam, lParam);
	
		if (handled)
			return 0;
	}

	return DefWindowProc(hWnd, message, wParam, lParam);
}
