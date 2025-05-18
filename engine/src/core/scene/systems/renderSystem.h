#pragma once

#include <pch.h>
#include "system.h"
#include <entt/entt.hpp>
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
		bool operator<(const materialID& other) const
		{
			if (shaderProgramID == other.shaderProgramID)
				return textureID < other.textureID;

			if (textureID == other.textureID)
				return shaderProgramID < other.shaderProgramID;

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

	struct dynamicRenderData
	{
		std::unique_ptr<dynamicArrayObject> EBO;
		std::unique_ptr<dynamicArrayObject> VBO;
		std::unique_ptr<vertexArrayObject> VAO;
		std::map<entityID, entityBoundaries> entityBoundaries;
	};

	class renderSystem : public system
	{
	private:
		fpsCamera mDefaultCamera;

		openglRenderer mRenderer;
		std::map<materialID, dynamicRenderData> mData;

		void resizeOnNeed(dynamicRenderData& renderData, const std::vector<vertex>& vbo, const std::vector<uint32_t> ebo);
		void deleteEntities(entt::registry& registry);
		void addEntities(entt::registry& registry);
		void updateData(entt::registry& registry);
		void render(entt::registry& registry);
	public:
		renderSystem(context ctx, fpsCamera defaultCamera);
		void onRender(entt::registry& registry);
		void onEvent(entt::registry& registry, std::shared_ptr<baseEvent> e);
		void onUpdate(entt::registry& registry);
		error checkError();
	};
}