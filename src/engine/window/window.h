#pragma once

#include <pch.h>
#include "../events/dispatcher.h"

namespace engine {
	class window {
	protected:
		std::string mName;
		std::uint32_t mWidth, mHeight;
		core::error mErr;
		bool mIsFullscreen;
		engine::eventDispatcher mDispatcher;
	public:
		window(
			const std::string& name, 
			std::uint32_t width, 
			std::uint32_t heigth, 
			bool isFullscreen);
		virtual core::error checkError();
		
		virtual ~window() = default;
		virtual core::error makeOpenglContext();
		window(const window& other) = default;
		window(window&& other) noexcept;
		window& operator=(const window& other);
		window& operator=(window&& other) noexcept;
		virtual void updateWindowState();
		virtual void swapBuffers() const;
	};
}