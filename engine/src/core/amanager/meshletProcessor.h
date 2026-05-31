#pragma once

#include <pch.h>
#include "platform/renderer/vertex.h"
#include "primitiveProcessor.h"

struct cgltf_data;

namespace engine
{
	std::vector<glm::vec4> calculateTangents(
		const std::vector<glm::vec4>& positions,
		const std::vector<glm::vec4>& normals,
		const std::vector<uint32_t>& indices
	);

	std::vector<uint32_t> repackPrimitives(
		const std::vector<uint8_t>& primitives,
		std::vector<meshlet>& meshlets
	);

	std::pair<glm::vec3, float> calculateBoundingSphere(const std::vector<glm::vec3>& positions);

	error remapMesh(primitive& prim, bool isSkinned);

	error generateMeshlets(
		const std::vector<glm::vec4>& positions,
		const std::vector<uint32_t>& indicies,
		std::vector<meshlet>& mOut,
		std::vector<uint8_t>& pOut,
		std::vector<uint32_t>& iOut,
		size_t maxVert, size_t maxTriangles, float coneWieght,
		float errorLevel,
		size_t targetIndexCount,
		uint32_t materialOffset,
		alphaModeType alphaType
	);

	error generateLodLevel(
		const std::vector<glm::vec4>& positions,
		const std::vector<uint32_t> i,
		std::vector<meshlet>& meshletsOut,
		std::vector<uint32_t>& indicesOut,
		std::vector<uint32_t>& repackedPrimitivesOut,
		size_t targetIndexCount,
		size_t maxVert,
		size_t maxTriangles,
		float coneWeight,
		float errorLevel,
		uint32_t materialOffset,
		alphaModeType alphaMode
	);

	withError<std::pair<std::vector<mesh>, std::vector<perMeshAttributes>>> proccessMeshes(const cgltf_data* data, size_t maxVert, size_t maxTriangles, float coneWeight, float errorLevel);
	std::pair<std::vector<animation>, std::vector<skin>> proccessAnimations(const cgltf_data* data);
}