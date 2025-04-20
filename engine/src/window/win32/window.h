#pragma once

#include <pch.h>
#include <WindowsX.h>
#include <tchar.h>
#include <hidusage.h>
#include <queue>
#include "window/window.h"
#include "events/events.h"

namespace engine
{
	class winApiWindow : public baseWindow
	{
	private:
		static void logLastError(const std::string& prefix);
		static std::mutex hwndTableMu;
		static std::map<HWND, winApiWindow*> hwndTable;
		static WNDCLASSEX wndClass;
		static std::once_flag isWindowClassCreated;
		static core::error createWndClassErr;
		static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
		static bool handleRawInput(winApiWindow* winApiInst, HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
		static bool handleCloseEvent(winApiWindow* winApiInst, HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
		static bool handleResizeEvent(winApiWindow* winApiInst, HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
		static bool handlePaintEvent(winApiWindow* winApiInst, HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

		void createWindowClass(const std::string& applicationName);
		void registerInputDevices();
		void createWindow();
		void pollRawInput();

		std::mutex mEvenetQueueMu;
		std::map<key, std::shared_ptr<baseEvent>> mKeyDown;
		std::queue<std::shared_ptr<baseEvent>> mEventQueue;

		std::string mApplicationName;
		HDC mHdc;
		HWND mHWnd;
		HGLRC mHrc;
	public:
		winApiWindow(context ctx, const std::string& name, std::uint32_t width, std::uint32_t heigth, bool isFullscreen, const std::string& applicationName, bool showCursor);

		~winApiWindow();
		winApiWindow(const winApiWindow& other) = delete;
		winApiWindow& operator=(const winApiWindow& other) = delete;

		void swapBuffers() const;
		void toggleCursor();
		core::error checkError();
		void startPolling();
		core::error makeOpenglContext();
		void pollInput();
		bool isKeyPressed(key);
	};
}
