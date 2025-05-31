#pragma once

#include <pch.h>
#include "base/context/context.h"
#include "core/events/events.h"

namespace engine
{
	class window
	{
	protected:
		std::string mName;
		std::uint32_t mWidth, mHeight;
		engine::error mErr;
		bool mIsFullscreen;
		bool mShowCursor;
		std::shared_ptr<context> mCtx;
	public:
		window(
			std::shared_ptr<context> ctx,
			const std::string& name,
			std::uint32_t width,
			std::uint32_t heigth,
			bool isFullscreen,
			bool showCuresor);
		virtual ~window() = default;

		window(const window&) = delete;
		window& operator=(const window&) = delete;

		virtual engine::error makeOpenglContext() = 0;
		virtual void startPolling() = 0;
		virtual void swapBuffers() const = 0;
		virtual engine::error checkError() = 0;
		virtual void toggleCursor() = 0;
		virtual void pollInput() = 0;
		virtual bool isKeyPressed(key) = 0;
	};
}