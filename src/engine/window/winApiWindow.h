#pragma once

#include <pch.h>
#include "window.h"
#include "../events/events.h"
#include "../events/dispatcher.h"
#include <tchar.h>

namespace engine {
	class winApiWindow : public engine::window {
	private:
		static WNDCLASSEX mWndClass;

		static std::once_flag mIsWindowClassCreated;
		static core::error mCreateWndClassErr;
		static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
		static void createWindowClass(const std::string& applicationName);
		
		std::string mApplicationName;
		int mNoCmdShow;
		HWND mHWnd;
		HDC mHdc;
		HGLRC mHrc;
	public:
		winApiWindow(const std::string& name, std::uint32_t width, std::uint32_t heigth, bool isFullscreen, const std::string& applicationName, int noCmdShow);
		~winApiWindow();
	
		winApiWindow(const winApiWindow& other);
		winApiWindow(winApiWindow&& other) noexcept;
		winApiWindow& operator=(const winApiWindow& other);
		winApiWindow& operator=(winApiWindow&& other) noexcept;
	
		core::error makeOpenglContext();
		void updateWindowState();
	};
}