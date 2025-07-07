#include <pch.h>
#include "window.h"

namespace engine
{
	bool winApiWindow::handleRawInput(winApiWindow* winApiInst, [[maybe_unused]] HWND hWnd, UINT message, [[maybe_unused]] WPARAM wParam, LPARAM lParam)
	{
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
						winApiInst->mEventQueue.push(std::make_shared<mouseMoveEvent>(mouseOffset{ raw->data.mouse.lLastX, raw->data.mouse.lLastY }));
					}
					else
					{
						const RAWKEYBOARD& kbd = raw->data.keyboard;

						bool isKeyDown = !(kbd.Flags & RI_KEY_BREAK);

						auto keyCode = fromRawKeyboard(raw);

						if (isKeyDown)
						{
							if (winApiInst->mKeyDown.find(keyCode) == winApiInst->mKeyDown.end())
							{
								winApiInst->mKeyDown[keyCode] = std::make_shared<keyDownEvent>(keyCode);
							}
						}
						else
						{
							if (winApiInst->mKeyDown.find(keyCode) != winApiInst->mKeyDown.end())
							{
								winApiInst->mKeyDown.erase(keyCode);
							}

							winApiInst->mEventQueue.push(std::make_shared<keyUpEvent>(keyUpEvent{ keyCode }));
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

	bool winApiWindow::handleCloseEvent(HWND hWnd, UINT message, [[maybe_unused]] WPARAM wParam, [[maybe_unused]] LPARAM lParam)
	{
		if (message == WM_CLOSE)
		{
			if (!DestroyWindow(hWnd))
			{
				logLastError("can't destroy window");
				return false;
			}

			return true;
		}

		return false;
	}

	bool winApiWindow::handleDestroyEvent(winApiWindow* winApiInst, HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		if (message == WM_DESTROY)
		{
			PostQuitMessage(0);

			if (winApiInst)
				winApiInst->mCtx->mEventDispatcher->queueEvent(std::make_shared<closeEvent>());

			return true;
		}

		return false;
	}

	bool winApiWindow::handleResizeEvent(winApiWindow* winApiInst, [[maybe_unused]] HWND hWnd, UINT message, [[maybe_unused]] WPARAM wParam, LPARAM lParam)
	{
		if (message == WM_SIZE)
		{
			winApiInst->mCtx->mEventDispatcher->queueEvent(std::make_shared<windowResizeEvent>(LOWORD(lParam), HIWORD(lParam)));

			winApiInst->mHeight = (HIWORD(lParam));
			winApiInst->mWidth = (LOWORD(lParam));

			return true;
		}

		return false;
	}

	bool winApiWindow::handlePaintEvent([[maybe_unused]] HWND hWnd, [[maybe_unused]] UINT message, [[maybe_unused]] WPARAM wParam, [[maybe_unused]] LPARAM lParam)
	{
		if (message == WM_PAINT)
			return true;

		return false;
	}

	LRESULT winApiWindow::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		if (handleCloseEvent(hWnd, message, wParam, lParam))
			return 0;

		if (handlePaintEvent(hWnd, message, wParam, lParam))
			return 0;

		{
			std::lock_guard<std::mutex> l{ hwndTableMu };
			auto pThis = hwndTable.find(hWnd);
			if (pThis != hwndTable.end() && pThis->first)
			{
				if (handleRawInput(pThis->second, hWnd, message, wParam, lParam))
					return 0;

				if (handleResizeEvent(pThis->second, hWnd, message, wParam, lParam))
					return 0;

				if (handleDestroyEvent(pThis->second, hWnd, message, wParam, lParam))
					return 0;
			}

			if (handleDestroyEvent(nullptr, hWnd, message, wParam, lParam))
				return 0;
		}

		return DefWindowProc(hWnd, message, wParam, lParam);
	}

	// Not used right know.
	void winApiWindow::pollRawInput()
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
}