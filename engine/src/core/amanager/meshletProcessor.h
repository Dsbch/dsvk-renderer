#pragma once

#include <pch.h>
#include "platform/renderer/vertex.h"

struct cgltf_data;

namespace engine
{
	void calculateTangents(
		std::vector<vertex>& v,
		const std::vector<uint32_t>& index
	);

	std::vector<uint32_t> repackPrimitives(
		const std::vector<uint8_t>& primitives,
		std::vector<meshlet>& meshlets
	);

	std::pair<glm::vec3, float> calculateBoundingSphere(const std::vector<vertex>& vertices);

	error remapMesh(
		const std::vector<vertex>& vertecies,
		const std::vector<uint32_t> indicies,
		std::vector<vertex>& vOut,
		std::vector<uint32_t>& iOut
	);

	error generateMeshlets(
		const std::vector<vertex>& vertecies,
		const std::vector<uint32_t>& indicies,
		std::vector<meshlet>& mOut,
		std::vector<uint8_t>& pOut,
		std::vector<uint32_t>& iOut,
		size_t maxVert, size_t maxTriangles, float coneWieght,
		float errorLevel,
		size_t targetIndexCount
	);

	withError<mesh> processMesh(const cgltf_data* data, size_t maxVert, size_t maxTriangles, float coneWeight, float errorLevel);
}