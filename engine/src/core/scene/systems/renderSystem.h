#pragma once

#include <pch.h>
#include <entt/entt.hpp>
#include "system.h"
#include "base/context/context.h"
#include "core/scene/scene.h"
#include "platform/renderer/vertex.h"
#include "platform/renderer/opengl/renderer.h"
#include "platform/renderer/opengl/arrayObject.h"
#include "platform/renderer/opengl/vertexBufferObject.h"

namespace engine
{
	typedef uint32_t entityID;

	struct materialID
	{
		uint32_t textureID;
		uint32_t shaderProgramID;
		bool operator<(const materialID& other)  const
		{
			return textureID < other.textureID && shaderProgramID < other.shaderProgramID;
		}
	};

	struct entityBoundaries
	{
		size_t fromVBO;
		size_t toVBO;

		size_t fromEBO;
		size_t toEBO;
	};

	template<class T>
	struct renderDataHandle
	{
		std::unique_ptr<T> mEBO;
		std::unique_ptr<T> mVBO;
		std::unique_ptr<vertexArrayObject> mVAO;
		std::map<entityID, entityBoundaries> mEntityBoundaries;
	};

	class renderSystem : public system
	{
	private:
		openglRenderer mRenderer;

		std::map<materialID, renderDataHandle<arrayObject>> mStaticData;
		std::map<materialID, renderDataHandle<dynamicArrayObject>> mDynamicData;

		void deleteEntities(entt::registry& registry);
		void resizeOnNeed(renderDataHandle<dynamicArrayObject>& renderData, const std::vector<vertex>& vbo, const std::vector<uint32_t> ebo);
		void addEntities(entt::registry& registry);
		void updateData(entt::registry& registry);
		void render(entt::registry& registry);
	public:
		renderSystem(context ctx);
		void onRender(entt::registry& registry);
		void onEvent(entt::registry& registry, std::shared_ptr<baseEvent> e);
		error checkError();
	};
}