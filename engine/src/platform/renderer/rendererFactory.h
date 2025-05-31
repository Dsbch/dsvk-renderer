#pragma once

#include <pch.h>

#include "opengl/arrayObject.h"
#include "opengl/renderer.h"
#include "opengl/shader.h"
#include "opengl/texture.h"
#include "opengl/vertexArrayObject.h"

namespace engine
{
	class rendererFactory
	{
	public:
		static std::unique_ptr<dynamicArrayObject> createDynamicArrayObject(size_t size, void* data)
		{
#ifdef OPENGL
			return std::make_unique<openglDynamicArrayObject>(size, data);
#endif // OPENGL
		}

		static std::unique_ptr<arrayObject> createArrayObject(size_t size, void* data)
		{
#ifdef OPENGL
			return std::make_unique<openglArrayObject>(size, data);
#endif // OPENGL
		}

		static std::unique_ptr<vertexArrayObject> createVertexArrayObject()
		{
#ifdef OPENGL
			return std::make_unique<openglVertexArrayObject>();
#endif // OPENGL
		}

		static std::unique_ptr<renderer> createRenderer(std::shared_ptr<context> ctx)
		{
#ifdef OPENGL
			return std::make_unique<openglRenderer>(ctx);
#endif // OPENGL
		}

		static std::shared_ptr<shaderProgram> createShader(const std::string& fragmestSrc, const std::string& vertexSrc)
		{
#ifdef OPENGL
			return std::make_unique<openglShaderProgram>(fragmestSrc, vertexSrc);
#endif // OPENGL
		}

		static std::shared_ptr<texture> createTexure(uint8_t* data, int width, int heigth, imageChannel channel)
		{
#ifdef OPENGL
			return std::make_unique<openglTexture>(data, width, heigth, channel);
#endif // OPENGL
		}
	};
}

