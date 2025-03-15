#pragma once

#include <pch.h>
#include "opengl/renderer.h"

namespace engine {
	class rendererFactory {
	public:
		static std::unique_ptr<engine::renderer> createRenderer()
		{
			return std::make_unique<engine::openglRenderer>();
#ifdef OPENGL
			return std::make_unique<engine::renderer>();
#endif // OPENGL
		}
	};
}