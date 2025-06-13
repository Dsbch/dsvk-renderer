#pragma once

#include <pch.h>
#include "system.h"
#include "base/context/context.h"
#include "platform/renderer/arrayObject.h"
#include "platform/renderer/renderer.h"

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

	struct meshBoundaries
	{
		size_t fromVBO;
		size_t toVBO;

		size_t fromEBO;
		size_t toEBO;
	};

	typedef uint32_t meshID;
	typedef size_t instanceAttrIndex;
	typedef size_t drawCommandIndex;
	typedef size_t instanceSlotID;

	struct drawCommand
	{
		drawElementsCommand command;
		size_t index;
	};

	struct gpuDrivenData
	{
		std::map<meshID, meshBoundaries> boundaries;
		std::unique_ptr<dynamicArrayObject> EBO;
		std::unique_ptr<dynamicArrayObject> VBO;

		std::unique_ptr<dynamicArrayObject> instanceBuffer;
		std::unique_ptr<vertexArrayObject> VAO;
		
		std::unique_ptr<dynamicArrayObject> indirectBuffer;
		std::map<meshID, drawCommand> drawCommands;
		
		std::map<entityID, instanceAttrIndex> instanceBufferIndex;
		std::map<meshID, instanceSlotID> occupiedSlots;
		std::queue<instanceSlotID> freeSlots;
		size_t instancesPerMesh;
		size_t meshesPerMaterial;
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

		void resizeOnNeed(gpuDrivenData&, const std::vector<vertex>& vbo, const std::vector<uint32_t> ebo, uint32_t meshUID);
		void addEntities(entt::registry& registry);
		void updateData(entt::registry& registry);
		void deleteEntities(entt::registry& registry);
		void render(entt::registry& registry, const renderer* renderer, const fpsCamera& camera);

		friend class renderSystems;
	};
}
