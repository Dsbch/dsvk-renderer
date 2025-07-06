#pragma once

#include <pch.h>
#include "platform/renderer/arrayObject.h"
#include "platform/renderer/renderer.h"
#include "platform/renderer/shader.h"
#include "platform/renderer/texture.h"
#include "platform/renderer/vertexArrayObject.h"

#ifdef OPENGL
#include "platform/renderer/opengl/arrayObject.h"
#include "platform/renderer/opengl/renderer.h"
#include "platform/renderer/opengl/shader.h"
#include "platform/renderer/opengl/texture.h"
#include "platform/renderer/opengl/vertexArrayObject.h"
#endif // !OPENGL


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

#ifdef VULKAN
			return nullptr;
#endif // VULKAN

		}

		static std::unique_ptr<arrayObject> createArrayObject(size_t size, void* data)
		{
#ifdef OPENGL
			return std::make_unique<openglArrayObject>(size, data);
#endif // OPENGL

#ifdef VULKAN
			return nullptr;
#endif // VULKAN
		}

		static std::unique_ptr<vertexArrayObject> createVertexArrayObject()
		{
#ifdef OPENGL
			return std::make_unique<openglVertexArrayObject>();
#endif // OPENGL


#ifdef VULKAN
			return nullptr;
#endif // VULKAN
		}

		static std::unique_ptr<renderer> createRenderer(std::shared_ptr<context> ctx)
		{
#ifdef OPENGL
			return std::make_unique<openglRenderer>(ctx);
#endif // OPENGL


#ifdef VULKAN
			return nullptr;
#endif // VULKAN
		}

		static std::shared_ptr<shaderProgram> createShader(const std::string& fragmestSrc, const std::string& vertexSrc)
		{
#ifdef OPENGL
			return std::make_unique<openglShaderProgram>(fragmestSrc, vertexSrc);
#endif // OPENGL


#ifdef VULKAN
			return nullptr;
#endif // VULKAN
		}

		static std::shared_ptr<texture> createTexure(uint8_t* data, int width, int heigth, imageChannel channel)
		{
#ifdef OPENGL
			return std::make_unique<openglTexture>(data, width, heigth, channel);
#endif // OPENGL


#ifdef VULKAN
			return nullptr;
#endif // VULKAN
		}

		static std::shared_ptr<cubeMap> createCubeMap(const std::array<uint8_t*, 6> data, int width, int heigth, imageChannel channel)
		{
#ifdef OPENGL
			return std::make_shared<openglCubeMap>(data, width, heigth, channel);
#endif // OPENGL


#ifdef VULKAN
			return nullptr;
#endif // VULKAN
		}
	};
}

