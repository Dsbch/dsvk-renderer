#pragma once

#include <pch.h>
#include "core/scene/systems/system.h"
#include "platform/renderer/arrayObject.h"
#include "platform/renderer/renderer.h"
#include <glm/gtc/type_ptr.inl>

namespace std {
	template <>
	struct std::hash<glm::vec3> {
		size_t operator()(const glm::vec3& v) const {
			size_t hx = std::hash<float>{}(v.x);
			size_t hy = std::hash<float>{}(v.y);
			size_t hz = std::hash<float>{}(v.z);
			return hx ^ (hy << 1) ^ (hz << 2);
		}
	};

	template <>
	struct std::hash<glm::mat4> {
		size_t operator()(const glm::mat4& mat) const {
			const float* data = glm::value_ptr(mat);
			size_t result = 0;
			for (int i = 0; i < 16; ++i)
				result ^= std::hash<float>{}(data[i]) << (i % 8);
			return result;
		}
	};
}


namespace engine
{
	size_t hashUniforms(const materialComponent::shaderUniformMap& uniforms);
	
	typedef uint32_t entityID;

	struct materialID
	{
		uint32_t textureID;
		uint32_t shaderProgramID;
		size_t uniformID;

		bool operator<(const materialID& other) const {
			if (shaderProgramID != other.shaderProgramID)
				return shaderProgramID < other.shaderProgramID;
			
			if (textureID != other.textureID)
				return textureID < other.textureID;

			return uniformID < other.uniformID;
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

	struct coreRenderData
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

	class coreRender : public system
	{
	public:
		coreRender(std::shared_ptr<context>);
		error checkError();
		void onUpdate(entt::registry& registry);
		void onRender(entt::registry& registry);
		void onEvent(entt::registry& registry, std::shared_ptr<baseEvent> e);
	private:
		std::map<materialID, coreRenderData> mData;

		void resizeOnNeed(coreRenderData&, const std::vector<vertex>& vbo, const std::vector<uint32_t> ebo, uint32_t meshUID);
		void addEntities(entt::registry& registry);
		void updateData(entt::registry& registry);
		void deleteEntities(entt::registry& registry);
		void render(entt::registry& registry, const renderer* renderer, const fpsCamera& camera);

		friend class renderSystems;
	};
}
