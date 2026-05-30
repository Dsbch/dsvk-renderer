#pragma once

#include <pch.h>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/mat3x3.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>

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

	struct animVertex
	{
		vertex vert;
		uint32_t joints[4];
		float weights[4];
	};

	struct transform
	{
		glm::vec3 translation;
		glm::vec3 scale;
		glm::quat rotation;
	};

	glm::mat4 toMat4(const transform& trs);

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
		transform modelTransform;
		uint32_t globalMaterialOffset;
		uint32_t jointIndex;
		uint32_t jointOffset;
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
		uint32_t alphaType;
		uint32_t localMaterialOffset;

		uint32_t indexBufferIndex;
		uint32_t indexBufferOffset;

		uint32_t vertexBufferIndex;
		uint32_t vertexBufferOffset;
		uint32_t vertexCount;

		uint32_t triangleBufferIndex;
		uint32_t triangleBufferOffset;
		uint32_t triangleCount;

		uint32_t perMeshBufferIndex;
		uint32_t perMeshBufferOffset;

		meshletBounds bounds;
	};

	template<class T>
	struct dataWithLodLevels
	{
		uint32_t second;
		uint32_t third;
		uint32_t fourth;
		std::vector<T> data;
	};

	struct mesh
	{
		std::vector<vertex> vertices;
		std::vector<animVertex> animVertices;
		dataWithLodLevels<uint32_t> indices;
		dataWithLodLevels<uint32_t> primitives;
		dataWithLodLevels<meshlet> meshlets;

		uint32_t meshHash = 0;
		uint32_t vertexHash = 0;
		uint32_t indexHash = 0;
		uint32_t primitiveHash = 0;
		uint32_t meshletHash = 0;

		void generateHashes()
		{
			if (vertexHash != 0 && meshletHash != 0 && meshHash != 0)
				return;

			const uint8_t* vertexPtr = vertices.size() == 0 ? reinterpret_cast<const uint8_t*>(animVertices.data()) : reinterpret_cast<const uint8_t*>(vertices.data());
			size_t size = vertices.size() == 0 ? animVertices.size() : vertices.size();
			size_t sizeOf = vertices.size() == 0 ? sizeof(animVertex) : sizeof(vertex);

			vertexHash = crc32(vertexPtr, size * sizeOf / sizeof(uint8_t));

			indexHash = crc32(reinterpret_cast<const uint8_t*>(indices.data.data()), indices.data.size() * sizeof(uint32_t) / sizeof(uint8_t));
			
			primitiveHash = crc32(reinterpret_cast<const uint8_t*>(primitives.data.data()), primitives.data.size() * sizeof(uint32_t) / sizeof(uint8_t));

			meshletHash = crc32(reinterpret_cast<const uint8_t*>(meshlets.data.data()), meshlets.second * sizeof(meshlet) / sizeof(uint8_t));

			meshHash = mergeCrc32({ vertexHash, meshletHash, indexHash, primitiveHash });
		}
	};

	enum class alphaModeType
	{
		opaque,
		blend,
		mask,
	};

	struct materialTextures
	{
		std::shared_ptr<const texture> albedo;
		std::shared_ptr<const texture> normal;
		std::shared_ptr<const texture> metallicRoughness;
		alphaModeType alphaMode;
	};

	struct materials
	{
		std::shared_ptr<const shader> pixelShader;
		std::vector<materialTextures> textures;

		uint32_t hash = 0;

		void generateHash()
		{
			if (hash != 0)
				return;

			std::vector<uint32_t> crcVals;
			for (const auto& t : textures)
			{
				crcVals.push_back(t.albedo->hash());
				crcVals.push_back(t.normal->hash());
				crcVals.push_back(t.metallicRoughness->hash());
			}

			hash = mergeCrc32(crcVals);
		}

		bool hasBlendMaterials() const
		{
			for (auto& matText : textures)
				if (matText.alphaMode == alphaModeType::blend)
					return true;

			return false;
		}
	};

	struct joint
	{
		glm::mat4 inverseBind;
		transform localTransform;
		bool isSkinJoint;
		int parentIdx;
	};

	struct skin
	{
		std::vector<joint> skinJoints;

		std::vector<glm::mat4> getJointMatrices();
	};

	enum animationType
	{
		tr,
		rt,
		sc,
	};

	enum interpolationType
	{
		linear,
		cubicspline,
		step,
	};

	struct channel
	{
		animationType aType;
		interpolationType iType;
		float currentTimeStamp;
		size_t skinIndex;
		size_t jointIndex;

		std::shared_ptr<const std::vector<float>> timestamps;
		std::shared_ptr<const std::vector<transform>> keyframes;
	};

	struct animation
	{
		std::string name;
		std::vector<channel> channels;

		void update(float deltaTime, std::shared_ptr<std::vector<skin>> skins);
	};

	struct perMeshAttributes
	{
		float  bsRadius;
		glm::vec3 bsCenter;

		uint32_t isSkinned;
		glm::mat4 meshLocalTransform;
		glm::mat4 meshGlobalTransform;
		glm::mat3 meshLocalNormal;
		glm::mat3 meshGlobalNormal;
	};

	struct animations
	{
		std::shared_ptr<std::vector<glm::mat4>> jointMatrices;
		std::shared_ptr<std::vector<skin>> skins;
		std::shared_ptr<std::vector<animation>> animations;
	};

	struct model
	{
		uint32_t id;
		perInstanceAttr instanceAttributes;
		std::shared_ptr<const std::vector<mesh>> meshData;
		std::shared_ptr<const std::vector<perMeshAttributes>> perMeshData;
		materials mat;
		animations anims;
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
static_assert(std::is_trivially_constructible_v<engine::transform>&& std::is_standard_layout_v<engine::transform>);
static_assert(std::is_trivially_constructible_v<engine::meshletShaderCMD>&& std::is_standard_layout_v<engine::meshletShaderCMD>);
static_assert(std::is_trivially_constructible_v<engine::meshlet>&& std::is_standard_layout_v<engine::meshlet>);
static_assert(std::is_trivially_constructible_v<engine::meshletBounds>&& std::is_standard_layout_v<engine::meshletBounds>);
static_assert(std::is_trivially_constructible_v<engine::perMeshAttributes>&& std::is_standard_layout_v<engine::perMeshAttributes>);
