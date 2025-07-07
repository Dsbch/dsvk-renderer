#pragma once

#include <pch.h>
#include "window.h"

namespace engine
{
	class windowFactory
	{
	public:
		static std::unique_ptr<window> createWindow(
			std::shared_ptr<context> ctx,
			const std::string& name,
			std::uint32_t width,
			std::uint32_t heigth,
			bool isFullscreen,
			const std::string& applicationName,
			bool showCursor
		);
	};
}