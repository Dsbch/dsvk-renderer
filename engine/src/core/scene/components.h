#pragma once

#include <pch.h>

namespace engine
{
	struct tagComponent
	{
		std::string tag;

		tagComponent(const std::string& tag)
			: tag(tag) {}
	};
}