#pragma once

#include <pch.h>

namespace engine {
	class renderer {
	public:
		renderer() = default;
		virtual ~renderer() = default;
		virtual std::string getVersion() const = 0;
		virtual core::error check() const = 0;
	};
}