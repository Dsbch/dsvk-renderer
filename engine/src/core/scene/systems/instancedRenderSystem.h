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
	typedef size_t instanceAttrIndex;

	struct instancedRenderData
	{
		uint32_t meshUID;

		mutable uint32_t instanceCount;
		mutable std::unique_ptr<dynamicArrayObject> EBO;
		mutable std::unique_ptr<dynamicArrayObject> VBO;
		mutable std::unique_ptr<vertexArrayObject> VAO;
		mutable std::unique_ptr<dynamicArrayObject> instanceAttributes;
		mutable std::map<entityID, instanceAttrIndex> boundaries;

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