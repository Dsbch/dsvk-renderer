#pragma once

#include <pch.h>
#include "platform/renderer/vertex.h"

struct cgltf_data;

namespace engine
{
	void calculateTangents(
		std::vector<vertex>& v,
		std::vector<animVertex>& animV,
		const std::vector<uint32_t>& indices
	);

	std::vector<uint32_t> repackPrimitives(
		const std::vector<uint8_t>& primitives,
		std::vector<meshlet>& meshlets
	);

	std::pair<glm::vec3, float> calculateBoundingSphere(const std::vector<vertex>& vertices, const std::vector<animVertex>& animVertices);

	error remapMesh(
		const std::vector<vertex>& vertecies,
		const std::vector<animVertex>& animVertecies,
		const std::vector<uint32_t> indicies,
		std::vector<vertex>& vOut,
		std::vector<animVertex>& animVOut,
		std::vector<uint32_t>& iOut
	);

	error generateMeshlets(
		const std::vector<vertex>& vertecies,
		const std::vector<animVertex>& animVertecies,
		const std::vector<uint32_t>& indicies,
		std::vector<meshlet>& mOut,
		std::vector<uint8_t>& pOut,
		std::vector<uint32_t>& iOut,
		size_t maxVert, size_t maxTriangles, float coneWieght,
		float errorLevel,
		size_t targetIndexCount
	);

	error generateLodLevel(
		const std::vector<vertex>& v,
		const std::vector<animVertex>& animV,
		const std::vector<uint32_t> i,
		mesh& crntMesh,
		size_t targetIndexCount,
		size_t maxVert,
		size_t maxTriangles,
		float coneWeight,
		float errorLevel
	);

	withError<std::pair<std::vector<mesh>, std::vector<perMeshAttributes>>> proccessMeshes(const cgltf_data* data, size_t maxVert, size_t maxTriangles, float coneWeight, float errorLevel);
	std::pair<std::vector<animation>, std::vector<skin>> proccessAnimations(const cgltf_data* data);
}