#pragma once

#include <pch.h>
#include "platform/renderer/arrayObject.h"
#include "platform/renderer/renderer.h"
#include "platform/renderer/shader.h"
#include "platform/renderer/texture.h"
#include "platform/renderer/vertexArrayObject.h"

namespace engine
{
	class rendererFactory
	{
	public:
		static std::unique_ptr<dynamicArrayObject> createDynamicArrayObject(size_t size, void* data)
		{
#ifdef VULKAN
			return nullptr;
#endif // VULKAN

		}

		static std::unique_ptr<arrayObject> createArrayObject(size_t size, void* data)
		{
#ifdef VULKAN
			return nullptr;
#endif // VULKAN
		}

		static std::unique_ptr<vertexArrayObject> createVertexArrayObject()
		{

#ifdef VULKAN
			return nullptr;
#endif // VULKAN
		}

		static std::unique_ptr<renderer> createRenderer(std::shared_ptr<context> ctx)
		{

#ifdef VULKAN
			return nullptr;
#endif // VULKAN
		}

		static std::shared_ptr<shaderProgram> createShader(const std::string& fragmestSrc, const std::string& vertexSrc)
		{

#ifdef VULKAN
			return nullptr;
#endif // VULKAN
		}

		static std::shared_ptr<texture> createTexure(uint8_t* data, int width, int heigth, imageChannel channel)
		{

#ifdef VULKAN
			return nullptr;
#endif // VULKAN
		}

		static std::shared_ptr<cubeMap> createCubeMap(const std::array<uint8_t*, 6> data, int width, int heigth, imageChannel channel)
		{

#ifdef VULKAN
			return nullptr;
#endif // VULKAN
		}
	};
}

