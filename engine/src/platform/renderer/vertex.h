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

	enum mehletVisibilityFlagBits : uint32_t {
		// At the end of second pass, all meshlets must have only two flags below.
		VISIBLE_BIT = 1 << 0,
		NOT_VISIBLE_BIT = 1 << 1,
		// Only between fist and second opaque pass.
		NOT_VISIBLE_NOW_BIT = 1 << 2,
	};

	struct meshlet
	{
		uint32_t alphaType;
		uint32_t localMaterialOffset;

		uint32_t indexBufferIndex;
		uint32_t indexBufferOffset;

		uint32_t weightBufferOffset;
		uint32_t weightBufferIndex;

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
		// Vertex attributes.
		// In pos fourth parameter is X texCoord.
		std::vector<glm::vec4> positions;
		// In normal fourth parameter is Y texCoord.
		std::vector<glm::vec4> normal;
		std::vector<glm::vec4> tangent;
		std::vector<glm::uvec4> jointIndices;
		std::vector<glm::vec4> weights;
		
		// Meshlets data.
		dataWithLodLevels<uint32_t> indices;
		dataWithLodLevels<uint32_t> primitives;
		dataWithLodLevels<meshlet> meshlets;

		uint32_t meshHash = 0;

		void generateHashes()
		{
			if (meshHash != 0)
				return;

			uint32_t posHash = crc32(reinterpret_cast<const uint8_t*>(positions.data()), positions.size() * sizeof(glm::vec4) / sizeof(uint8_t));
			uint32_t normalHash = crc32(reinterpret_cast<const uint8_t*>(normal.data()), normal.size() * sizeof(glm::vec4) / sizeof(uint8_t));
			uint32_t tangentHash = crc32(reinterpret_cast<const uint8_t*>(tangent.data()), tangent.size() * sizeof(glm::vec4) / sizeof(uint8_t));
			uint32_t jointsIndicesHash = crc32(reinterpret_cast<const uint8_t*>(jointIndices.data()), jointIndices.size() * sizeof(glm::uvec4) / sizeof(uint8_t));
			uint32_t weightsHash = crc32(reinterpret_cast<const uint8_t*>(weights.data()), weights.size() * sizeof(glm::vec4) / sizeof(uint8_t));

			uint32_t vertexHash = mergeCrc32( {posHash, normalHash, tangentHash, jointsIndicesHash, weightsHash } );

			uint32_t indexHash = crc32(reinterpret_cast<const uint8_t*>(indices.data.data()), indices.data.size() * sizeof(uint32_t) / sizeof(uint8_t));
			
			uint32_t primitiveHash = crc32(reinterpret_cast<const uint8_t*>(primitives.data.data()), primitives.data.size() * sizeof(uint32_t) / sizeof(uint8_t));

			uint32_t meshletHash = crc32(reinterpret_cast<const uint8_t*>(meshlets.data.data()), meshlets.second * sizeof(meshlet) / sizeof(uint8_t));

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

		std::vector<glm::mat4> getJointMatrices() const;
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
		void update(float deltaTime, std::vector<skin>& skins);
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

		uint32_t width;
		uint32_t height;
	};

	struct pushConstants
	{
		uint32_t commandBufferOffset;
		uint32_t meshletCount;
		uint32_t passNumber;
		uint32_t hzbBufferLength;
	};

	struct computePushConstants
	{
		uint32_t hzbMipLevel;
		uint32_t width;
		uint32_t height;
	};

	struct lineVertex
	{
		glm::vec3 position;
	};
}

static_assert(std::is_trivially_constructible_v<engine::perInstanceAttr>&& std::is_standard_layout_v<engine::perInstanceAttr>);
static_assert(std::is_trivially_constructible_v<engine::transform>&& std::is_standard_layout_v<engine::transform>);
static_assert(std::is_trivially_constructible_v<engine::meshletShaderCMD>&& std::is_standard_layout_v<engine::meshletShaderCMD>);
static_assert(std::is_trivially_constructible_v<engine::meshlet>&& std::is_standard_layout_v<engine::meshlet>);
static_assert(std::is_trivially_constructible_v<engine::meshletBounds>&& std::is_standard_layout_v<engine::meshletBounds>);
static_assert(std::is_trivially_constructible_v<engine::perMeshAttributes>&& std::is_standard_layout_v<engine::perMeshAttributes>);
