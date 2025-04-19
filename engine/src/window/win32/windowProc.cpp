#include <pch.h>
#include "window.h"

bool engine::winApiWindow::handleRawInput(engine::winApiWindow* winApiInst, HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	auto keyCode = engine::fromWinApiMouse(message);

	POINT cursorPos;
	cursorPos.x = GET_X_LPARAM(lParam);
	cursorPos.y = GET_Y_LPARAM(lParam);

	switch (message)
	{
	case WM_INPUT:
	{
		UINT dataSize = 0;
		if (GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, NULL, &dataSize, sizeof(RAWINPUTHEADER)) == (UINT)-1)
		{
			logLastError("GetRawInputData: ");
		}

		if (dataSize > 0)
		{
			void* rawData = alloca(dataSize);
			if (GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, rawData, &dataSize, sizeof(RAWINPUTHEADER)) == dataSize)
			{
				RAWINPUT* raw = static_cast<RAWINPUT*>(rawData);

				std::lock_guard<std::mutex> m(winApiInst->mEvenetQueueMu);
				if (raw->header.dwType == RIM_TYPEMOUSE)
				{
					winApiInst->mEventQueue.push(std::make_shared<engine::mouseMoveEvent>(engine::mouseOffset{ raw->data.mouse.lLastX, raw->data.mouse.lLastY }));
				}
				else
				{
					const RAWKEYBOARD& kbd = raw->data.keyboard;

					bool isKeyDown = !(kbd.Flags & RI_KEY_BREAK);

					auto keyCode = engine::fromRawKeyboard(raw);

					if (isKeyDown)
					{
						if (winApiInst->mKeyDown.find(keyCode) == winApiInst->mKeyDown.end())
						{
							winApiInst->mKeyDown[keyCode] = std::make_shared<engine::keyDownEvent>(keyCode);
						}
					}
					else
					{
						if (winApiInst->mKeyDown.find(keyCode) != winApiInst->mKeyDown.end())
						{
							winApiInst->mKeyDown.erase(keyCode);
						}

						winApiInst->mEventQueue.push(std::make_shared<engine::keyUpEvent>(engine::keyUpEvent{ keyCode }));
					}
				}
			}
			else
			{
				logLastError("GetRawInputData: ");
			}
		}

		return true;
	}
	}

	return false;
}

bool engine::winApiWindow::handleCloseEvent(engine::winApiWindow* winApiInst, HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (message == WM_CLOSE)
	{
		if (!DestroyWindow(hWnd))
		{
			logLastError("cant destroy window");
			return false;
		}

		return true;
	}

	if (message == WM_DESTROY)
	{
		PostQuitMessage(0);
		winApiInst->mCtx.getDispatcher()->queueEvent(std::make_shared<engine::closeEvent>());
		return true;
	}

	return false;
}

bool engine::winApiWindow::handleResizeEvent(engine::winApiWindow* winApiInst, HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (message == WM_SIZE)
	{
		winApiInst->mCtx.getDispatcher()->queueEvent(std::make_shared<engine::windowResizeEvent>(LOWORD(lParam), HIWORD(lParam)));
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
	hwndTableMu.lock();
	auto pThis = hwndTable.find(hWnd);
	auto end = hwndTable.end();
	hwndTableMu.unlock();

	if (pThis != end)
	{
		bool handled = false;

		handled |= handleRawInput(pThis->second, hWnd, message, wParam, lParam);
		handled |= handleResizeEvent(pThis->second, hWnd, message, wParam, lParam);
		handled |= handlePaintEvent(pThis->second, hWnd, message, wParam, lParam);
		handled |= handleCloseEvent(pThis->second, hWnd, message, wParam, lParam);

		if (handled)
			return 0;
	}

	return DefWindowProc(hWnd, message, wParam, lParam);
}

// Not used right know.
void engine::winApiWindow::pollRawInput()
{
	UINT size = 0;
	if (GetRawInputBuffer(nullptr, &size, sizeof(RAWINPUTHEADER)) == (UINT)-1)
	{
		logLastError("GetRawInputBuffer: ");
	}

	if (size > 0)
	{
		auto buffer = alloca(size);
		PRAWINPUT raw = static_cast<PRAWINPUT>(buffer);

		UINT count = 0;

		if (count = GetRawInputBuffer(raw, &size, sizeof(RAWINPUTHEADER)); count == (UINT)-1)
		{
			logLastError("GetRawInputBuffer: ");
		}

		for (UINT i = 0; i < count; ++i)
		{
			if (raw->header.dwType == RIM_TYPEMOUSE) {
				LOGINFO("Mouse Move: X={}, Y={}", raw->data.mouse.lLastX, raw->data.mouse.lLastY);
			}

			raw = reinterpret_cast<PRAWINPUT>(
				reinterpret_cast<BYTE*>(raw) + raw->header.dwSize
				);
		}
	}
}
