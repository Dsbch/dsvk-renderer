#pragma once

#include <pch.h>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/mat3x3.hpp>

#include "shader.h"

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

	struct instanceAttributes
	{
		glm::mat4 modelMatrix;
	};

	struct mesh
	{
		std::vector<vertex> vertexBuffer;
		std::vector<uint32_t> indexBuffer;

		bool isMeshlets;
		std::vector<uint32_t> primitiveBuffer;
		std::vector<uint32_t> vertexIndexBuffer;
	};

	struct meshHandle
	{
		std::array<mesh, 4> lodLevels;

		uint32_t getHesh() const
		{
			if (lodLevels.size() == 0)
				return 0;

			return crc32(reinterpret_cast<const uint8_t*>(lodLevels.front().vertexBuffer.data()), lodLevels.front().vertexBuffer.size());
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
		material mat;
		meshHandle mesh;
		instanceAttributes instanceAttributes;
	};
}

static_assert(std::is_trivially_constructible_v<engine::vertex>&& std::is_standard_layout_v<engine::vertex>);
static_assert(std::is_trivially_constructible_v<engine::instanceAttributes>&& std::is_standard_layout_v<engine::instanceAttributes>);
