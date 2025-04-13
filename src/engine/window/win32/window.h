#pragma once

#include <pch.h>
#include <WindowsX.h>
#include <tchar.h>
#include <hidusage.h>
#include "../window.h"
#include "../../events/events.h"

namespace engine {
	class winApiWindow : public engine::baseWindow {
	private:
		static void logLastError(const std::string& prefix);
		static std::mutex hwndTableMu;
		static std::map<HWND, winApiWindow*> hwndTable;
		static WNDCLASSEX wndClass;
		static std::once_flag isWindowClassCreated;
		static core::error createWndClassErr;
		static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
		static bool handleRawInput(engine::winApiWindow* winApiInst, HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
		static bool handleCloseEvent(engine::winApiWindow* winApiInst, HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
		static bool handleResizeEvent(engine::winApiWindow* winApiInst, HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
		static bool handlePaintEvent(engine::winApiWindow* winApiInst, HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
		
		void createWindowClass(const std::string& applicationName);
		void registerInputDevices();
		void createWindow();

		std::map<engine::key, engine::keyUpEvent> mKeyUp;
		std::map<engine::key, engine::keyDownEvent> mKeyDown;

		std::string mApplicationName;
		HDC mHdc;
		HWND mHWnd;
		HGLRC mHrc;
	public:
		winApiWindow(engine::context ctx, const std::string& name, std::uint32_t width, std::uint32_t heigth, bool isFullscreen, const std::string& applicationName, bool showCursor);
	
		~winApiWindow();
		winApiWindow(const winApiWindow& other) = delete;
		winApiWindow& operator=(const winApiWindow& other) = delete;
	
		void swapBuffers() const;
		void toggleCursor();
		core::error checkError();
		void startPolling();
		core::error makeOpenglContext();
	};
}
