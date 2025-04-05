#pragma once

#include <pch.h>
#include "../context/context.h"

namespace engine {
	class baseWindow {
	protected:
		std::string mName;
		std::uint32_t mWidth, mHeight;
		core::error mErr;
		bool mIsFullscreen;
		bool mShowCursor;
		engine::context mCtx;
	public:
		baseWindow(
			engine::context ctx,
			const std::string& name,
			std::uint32_t width,
			std::uint32_t heigth,
			bool isFullscreen,
			bool showCuresor);
		virtual ~baseWindow() = default;
		
		virtual core::error makeOpenglContext() = 0;
		virtual void swapBuffers() const = 0;
		virtual void updateWindowState() = 0;
		virtual core::error checkError() = 0;
		virtual void toggleCursor() = 0;
		virtual void dispatchInput() = 0;
	};
}