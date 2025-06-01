#pragma once

#include <pch.h>
#include "system.h"
#include "base/context/context.h"
#include "dynamicRenderSystem.h"

namespace engine
{
	typedef uint32_t meshID;
	typedef size_t instanceAttrIndex;

	struct dataBoundries
	{
		std::map<entityID, instanceAttrIndex> perInstanceBoundries;

		meshBoundaries boundries;
		drawElementsCommand drawCommand;
		size_t indirectBufferIndex;
	};

	struct gpuDrivenData
	{
		std::map<meshID, dataBoundries> boundaries;
		
		std::unique_ptr<dynamicArrayObject> EBO;
		std::unique_ptr<dynamicArrayObject> VBO;
		std::unique_ptr<dynamicArrayObject> instanceAttributes;
		std::unique_ptr<vertexArrayObject> VAO;
		std::unique_ptr<dynamicArrayObject> indirectBuffer;
	};

	class gpuDrivenRenderSystem : public system
	{
	public:
		gpuDrivenRenderSystem(std::shared_ptr<context>);
		error checkError();
		void onUpdate(entt::registry& registry);
		void onRender(entt::registry& registry);
		void onEvent(entt::registry& registry, std::shared_ptr<baseEvent> e);
	private:
		std::map<materialID, gpuDrivenData> mData;

		void resizeOnNeed(gpuDrivenData&, const std::vector<vertex>& vbo, const std::vector<uint32_t> ebo);
		void addEntities(entt::registry& registry);
		void updateData(entt::registry& registry);
		void deleteEntities(entt::registry& registry);
		void render(entt::registry& registry, const renderer* renderer, const fpsCamera& camera);

		friend class renderSystems;
	};
}
