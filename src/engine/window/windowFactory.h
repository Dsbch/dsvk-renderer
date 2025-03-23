#pragma once

#include <pch.h>
#include "win32/window.h"

namespace engine {
	class windowFactory {
	public:
		static std::unique_ptr<engine::window> createWindow(const std::string& name, std::uint32_t width, std::uint32_t heigth, bool isFullscreen, const std::string& applicationName, bool showCursor)
		{
#ifdef WIN32
			return std::make_unique<engine::winApiWindow>(name, width, heigth, isFullscreen, applicationName, showCursor);
#endif // WIN32
		}
	};
}