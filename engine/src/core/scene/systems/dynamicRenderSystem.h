#pragma once

#include <pch.h>
#include "system.h"
#include <entt/entt.hpp>
#include "core/scene/scene.h"
#include "platform/renderer/vertex.h"
#include "platform/renderer/renderer.h"
#include "platform/renderer/arrayObject.h"
#include "platform/renderer/vertexArrayObject.h"

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

	class dynamicRenderSystem : public system
	{
	private:
		std::map<materialID, dynamicRenderData> mData;

		void resizeOnNeed(dynamicRenderData& renderData, const std::vector<vertex>& vbo, const std::vector<uint32_t> ebo);
		void deleteEntities(entt::registry& registry);
		void addEntities(entt::registry& registry);
		void updateData(entt::registry& registry);
		void render(entt::registry& registry, const renderer* renderer, const fpsCamera& camera);
	public:
		dynamicRenderSystem(std::shared_ptr<context> ctx);
		void onRender(entt::registry& registry) {};
		void onEvent(entt::registry& registry, std::shared_ptr<baseEvent> e);
		void onUpdate(entt::registry& registry);
		error checkError();

		friend class renderSystems;
	};
}