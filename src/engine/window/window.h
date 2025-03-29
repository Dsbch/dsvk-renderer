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
		bool mShowCursor;
		std::shared_ptr<engine::eventDispatcher> mDispatcher;
		engine::context mCtx;
	public:
		window(
			engine::context ctx,
			std::shared_ptr<engine::eventDispatcher> dispatcher,
			const std::string& name,
			std::uint32_t width,
			std::uint32_t heigth,
			bool isFullscreen,
			bool showCuresor);
		
		virtual ~window() = default;
		window(const window& other) = default;
		window(window&& other) noexcept;
		window& operator=(window&& other) noexcept;
		virtual core::error makeOpenglContext() = 0;
		virtual void swapBuffers() const = 0;
		virtual void updateWindowState() = 0;
		virtual core::error checkError() = 0;
		virtual void toggleCursor() = 0;
	};
}