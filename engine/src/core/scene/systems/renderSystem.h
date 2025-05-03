#pragma once

#include <pch.h>
#include <entt/entt.hpp>
#include "system.h"
#include "base/context/context.h"
#include "core/scene/scene.h"
#include "platform/renderer/opengl/renderer.h"
#include "platform/renderer/opengl/arrayObject.h"
#include "platform/renderer/opengl/vertexBufferObject.h"

namespace engine
{
	class arrayObjectHandle
	{
	private:
		struct asd {
			size_t begin;
			size_t end;
		};

		std::map<uint32_t, asd> mAsd;
	public:

	};

	class renderSystem : public system
	{
	private:
		openglRenderer mRenderer;
		
		struct materialID
		{
			uint32_t textureID;
			uint32_t shaderProgramID;
		};

		std::map<materialID, std::unique_ptr<arrayObjectHandle>> mVboData;
		std::map<uint32_t, std::unique_ptr<vertexArrayObject>> mVao;
	public:

		renderSystem(context ctx);
		void onRender(entt::registry& registry);
		void onEvent(entt::registry& registry, std::shared_ptr<baseEvent> e);
		error checkError();
	};
}