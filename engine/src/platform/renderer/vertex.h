#pragma once

#include <pch.h>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/mat3x3.hpp>

#include "shader.h"
#include "texture.h"

namespace engine
{
	struct vertex
	{
		glm::vec3 position;
		float _pad0;

		glm::vec2 textureCoords;
		glm::vec2 _pad1;

		glm::vec3 normal;
		float _pad2;

		glm::vec3 tangent;
		float _pad3;
	};
	
	// Task shader buffer, to get meshlet and instanceAttrs.
	struct meshletToInstance
	{
		uint32_t instanceIndex;
		uint32_t instanceOffset;

		uint32_t meshletIndex;
		uint32_t meshletOffset;
	};

	struct perInstanceAttr
	{
		// TODO: figure out how to handle index updates for textures.
		uint32_t albedoIndex;
		uint32_t roughnessIndex;
		uint32_t normalIndex;
		uint32_t metalicIndex;
		uint32_t aoIndex;

		uint32_t _pad0[3];

		glm::mat4 modelMatrix;
	};

	struct meshlet
	{
		uint32_t indexBufferIndex;
		uint32_t indexBufferOffset;

		uint32_t vertexBufferIndex;
		uint32_t vertexBufferOffset;
		uint32_t vertexCount;

		uint32_t triangleBufferIndex;
		uint32_t triangleBufferOffset;
		uint32_t triangleCount;
	};


	struct mesh
	{
		std::shared_ptr<std::vector<vertex>> vertexBuffer;
		std::shared_ptr<std::vector<uint32_t>> indexBuffer;
		std::shared_ptr<std::vector<uint32_t>> primitiveBuffer;
		std::shared_ptr<std::vector<meshlet>> meshletBuffer;
	};

	struct lodMesh
	{
		std::array<mesh, 4> lodLevels;

		uint32_t hash = 0;

		uint32_t getHash()
		{
			if (lodLevels.size() == 0)
				return 0;

			if (hash != 0)
				return hash;

			hash = crc32(reinterpret_cast<const uint8_t*>(lodLevels.front().vertexBuffer->data()), lodLevels.front().vertexBuffer->size());

			return hash;
		}
	};

	struct material
	{
		std::shared_ptr<shader> pixelShader;

		std::shared_ptr<texture> albedoTexture;
		std::shared_ptr<texture> roughnessTexture;
		std::shared_ptr<texture> normalTexture;
		std::shared_ptr<texture> metalicTexture;
		std::shared_ptr<texture> aoTexture;
	};

	struct model
	{
		uint32_t id;
		material mat;
		lodMesh mesh;
		perInstanceAttr instanceAttributes;
	};
}

static_assert(std::is_trivially_constructible_v<engine::vertex>&& std::is_standard_layout_v<engine::vertex>);
static_assert(std::is_trivially_constructible_v<engine::perInstanceAttr>&& std::is_standard_layout_v<engine::perInstanceAttr>);
