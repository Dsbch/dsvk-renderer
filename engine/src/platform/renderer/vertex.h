#pragma once

#include <pch.h>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/mat3x3.hpp>
#include <glm/gtx/quaternion.hpp>

#include "shader.h"
#include "texture.h"

namespace engine
{
	typedef uint32_t entityHash;
	typedef uint32_t pixelShaderHash;
	typedef uint32_t meshHash;
	typedef uint32_t textureHash;

	struct vertex
	{
		glm::vec3 position;
		glm::vec2 textureCoords;
		glm::vec3 normal;
		glm::vec4 tangent;
	};

	struct transform
	{
		glm::vec3 translation;
		glm::vec3 scale;
		glm::quat rotation;
	};

	// Task/Amplification shader buffer.
	struct meshletShaderCMD
	{
		uint32_t instanceIndex;
		uint32_t instanceOffset;

		uint32_t meshletIndex;
		uint32_t meshletOffset1;
		uint32_t meshletOffset2;
		uint32_t meshletOffset3;
		uint32_t meshletOffset4;
	};

	struct perInstanceAttr
	{
		glm::vec3 bsWorldCenter;
		float  bsWorldRadius;

		transform modelTransform;

		uint32_t albedoIndex;
		uint32_t normalIndex;
		uint32_t metallicRoughnesIndex;
	};

	struct meshletBounds
	{
		/* bounding sphere, useful for frustum and occlusion culling */
		glm::vec3 center;
		float radius;
		/* normal cone, useful for backface culling */
		glm::vec3 coneAxis;
		float coneCutoff; /* = cos(angle/2) */
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

		meshletBounds bounds;
	};

	template<class T>
	struct dataWithLodLevels
	{
		uint32_t second;
		uint32_t third;
		uint32_t fourth;
		std::shared_ptr<std::vector<T>> data;
	};

	struct mesh
	{
		std::shared_ptr<std::vector<vertex>> vertex;
		dataWithLodLevels<uint32_t> index;
		dataWithLodLevels<uint32_t> primitive;
		dataWithLodLevels<meshlet> mesh;

		glm::vec3 bsCenter;
		float bsRadius;

		uint32_t hash = 0;

		uint32_t generateHash()
		{
			if (hash != 0)
				return hash;

			hash = crc32(reinterpret_cast<const uint8_t*>(vertex->data()), vertex->size() * sizeof(vertex) / sizeof(uint8_t));

			return hash;
		}

		uint32_t getHash() const
		{
			return hash;
		}
	};

	struct materialTextures
	{
		std::shared_ptr<texture> albedoAtlas;
		std::shared_ptr<texture> normalAtlas;
		std::shared_ptr<texture> metalicRoughnesAtlas;
	};

	struct material
	{
		std::shared_ptr<shader> pixelShader;
		materialTextures textures;
	};

	struct model
	{
		uint32_t id;
		perInstanceAttr instanceAttributes;
		mesh meshData;
		material mat;
	};

	struct frustum
	{
		glm::vec3 worldFrontN;
		float frontDistance;
		glm::vec3 worldBackN;
		float backDistance;
		glm::vec3 worldRightN;
		float rightDistance;
		glm::vec3 worldLeftN;
		float leftDistance;
		glm::vec3 worldTopN;
		float topDistance;
		glm::vec3 worldBottomN;
		float bottomDistance;
	};

	struct preDrawData
	{
		glm::mat4 debugViewProjection;

		uint32_t useDebugCamera;
		
		glm::vec3 cameraFront;
		glm::vec3 cameraPos;
		glm::vec3 cameraUp;
		glm::mat4 view;
		
		glm::mat4 projection;
		glm::mat4 viewProjection;

		frustum cameraFrustum;
		float deltaTime;
	};

	struct pushConstants
	{
		uint32_t commandBufferOffset;
		uint32_t meshletCount;
	};
	
	struct lineVertex
	{
		glm::vec3 position;
	};
}

static_assert(std::is_trivially_constructible_v<engine::vertex>&& std::is_standard_layout_v<engine::vertex>);
static_assert(std::is_trivially_constructible_v<engine::perInstanceAttr>&& std::is_standard_layout_v<engine::perInstanceAttr>);
