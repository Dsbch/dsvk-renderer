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

	struct materialOffset
	{
		uint32_t albedo;
		uint32_t roughness;
		uint32_t normal;
		uint32_t metalic;
		uint32_t ao;
	};

	struct bindlessOffset
	{
		uint32_t bufferIndex;
		uint32_t offset;
	};

	struct meshOffset
	{
		bindlessOffset vertex;
		bindlessOffset index;
		bindlessOffset primitive;
		bindlessOffset meshlet;
	};

	struct instanceAttributes
	{
		materialOffset textureOffset;
		meshOffset meshOffset;
		glm::mat4 modelMatrix;
	};

	struct meshlet
	{
		/* offsets within meshlet_vertices and meshlet_triangles arrays with meshlet data */
		uint32_t vertex_offset;
		uint32_t triangle_offset;

		/* number of vertices and triangles used in the meshlet; data is stored in consecutive range defined by offset and count */
		uint32_t vertex_count;
		uint32_t triangle_count;
	};

	struct mesh
	{
		std::shared_ptr<std::vector<vertex>> vertexBuffer;
		std::vector<uint32_t> indexBuffer;
		std::vector<uint32_t> primitiveBuffer;
		std::vector<meshlet> meshletBuffer;
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
		instanceAttributes instanceAttributes;
	};
}

static_assert(std::is_trivially_constructible_v<engine::vertex>&& std::is_standard_layout_v<engine::vertex>);
static_assert(std::is_trivially_constructible_v<engine::instanceAttributes>&& std::is_standard_layout_v<engine::instanceAttributes>);
