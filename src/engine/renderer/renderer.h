#pragma once

#include <pch.h>
#include "../events/events.h"

namespace engine {
	class renderer {
	public:
		renderer() = default;
		virtual ~renderer() = default;
		virtual std::string getVersion() const = 0;
		virtual core::error check() const = 0;
		virtual void changeViewPort(const engine::windowResizeEvent& e) const = 0;
		virtual void render() const = 0;
	};
}