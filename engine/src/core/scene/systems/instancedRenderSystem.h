#pragma once

#include <pch.h>
#include <entt/entt.hpp>
#include "dynamicRenderSystem.h"
#include "core/scene/scene.h"
#include "platform/renderer/vertex.h"
#include "platform/renderer/renderer.h"
#include "platform/renderer/arrayObject.h"
#include "platform/renderer/vertexArrayObject.h"

namespace engine
{
	struct instancedEntityBoundaries
	{
		size_t fromVBO;
		size_t toVBO;

		size_t fromEBO;
		size_t toEBO;

		size_t fromPerInstAttr;
		size_t toPerInstAttr;
	};

	struct instancedRenderData
	{
		uint32_t meshUID;

		mutable uint32_t instanceCount;
		mutable std::unique_ptr<dynamicArrayObject> EBO;
		mutable std::unique_ptr<dynamicArrayObject> VBO;
		mutable std::unique_ptr<vertexArrayObject> VAO;
		mutable std::unique_ptr<dynamicArrayObject> perInstanceAttrs;
		mutable std::map<uint32_t, instancedEntityBoundaries> boundaries;

		bool operator<(const instancedRenderData& other)  const
		{
			return meshUID < other.meshUID;
		}
	};

	class instancedRenderSystem : public system
	{
	public:
		instancedRenderSystem(std::shared_ptr<context> ctx);

		error checkError();
		void onRender(entt::registry& registry) {};
		void onEvent(entt::registry& registry, std::shared_ptr<baseEvent> e);
		void onUpdate(entt::registry& registry);
	private:
		void updateData(entt::registry& registry);
		void deleteEntities(entt::registry& registry);
		void addEntities(entt::registry& registry);
		void render(entt::registry& registry, const renderer* renderer, const fpsCamera& camera);
		void resizeOnNeed(const instancedRenderData&);

		std::map<materialID, std::set<instancedRenderData>> mData;

		friend class renderSystems;
	};
}