#pragma once

#include <pch.h>
#include <WindowsX.h>
#include <tchar.h>
#include "../window.h"
#include "../../events/events.h"
#include "../../events/dispatcher.h"

namespace engine {
	class winApiWindow : public engine::window {
	private:
		static std::map<HWND, winApiWindow*> hwndTable;
		static WNDCLASSEX wndClass;
		static std::once_flag isWindowClassCreated;
		static core::error createWndClassErr;
		static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
		static bool handleKeyboardEvent(engine::winApiWindow* winApiInst, HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
		static bool handleMouseEvent(engine::winApiWindow* winApiInst, HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
		static bool handleCloseEvent(engine::winApiWindow* winApiInst, HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
		static bool handleResizeEvent(engine::winApiWindow* winApiInst, HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
		static bool handlePaintEvent(engine::winApiWindow* winApiInst, HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
		
		void createWindowClass(const std::string& applicationName);
		std::string mApplicationName;
		HWND mHWnd;
		HDC mHdc;
		HGLRC mHrc;
	public:
		winApiWindow(engine::context ctx, std::shared_ptr<engine::eventDispatcher> dispatcher, const std::string& name, std::uint32_t width, std::uint32_t heigth, bool isFullscreen, const std::string& applicationName, bool showCursor);
		~winApiWindow();
	
		winApiWindow(const winApiWindow& other);
		winApiWindow(winApiWindow&& other) noexcept;
		winApiWindow& operator=(const winApiWindow& other);
		winApiWindow& operator=(winApiWindow&& other) noexcept;
	
		core::error makeOpenglContext();
		void updateWindowState();
		void swapBuffers() const;
		void toggleCursor();
		core::error checkError();
	};
}