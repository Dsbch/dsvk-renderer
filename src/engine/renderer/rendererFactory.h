#pragma once

#include <pch.h>
#include "opengl/renderer.h"

namespace engine {
	class rendererFactory {
	public:
		static std::unique_ptr<engine::renderer> createRenderer()
		{
#ifdef OPENGL
			return std::make_unique<engine::openglRednerer>();
#endif // OPENGL
			return std::make_unique<engine::renderer>();
		}
	};
}