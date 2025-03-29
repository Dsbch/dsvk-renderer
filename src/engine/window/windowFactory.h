#pragma once

#include <pch.h>
#include "win32/window.h"

namespace engine {
	class windowFactory {
	public:
		static std::unique_ptr<engine::window> createWindow(
			engine::context ctx,
			std::shared_ptr<engine::eventDispatcher> dispatcher,
			const std::string& name,
			std::uint32_t width,
			std::uint32_t heigth,
			bool isFullscreen,
			const std::string& applicationName,
			bool showCursor
			)
		{
#ifdef WIN32
			return std::make_unique<engine::winApiWindow>(ctx, dispatcher, name, width, heigth, isFullscreen, applicationName, showCursor);
#endif // WIN32
		}
	};
}