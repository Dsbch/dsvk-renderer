#pragma once

#include <pch.h>
#include "platform/renderer/vertex.h"

struct cgltf_data;

namespace engine
{
	void calculateTangents(
		const glm::vec3* positions,
		const glm::vec3* normals,
		const glm::vec2* textCoords,
		size_t verticesLen,
		size_t stride,
		const std::vector<uint32_t>& indices,
		glm::vec4* outTangents
	);

	std::vector<uint32_t> repackPrimitives(
		const std::vector<uint8_t>& primitives,
		std::vector<meshlet>& meshlets
	);

	std::pair<glm::vec3, float> calculateBoundingSphere(const glm::vec3* positions, size_t verticesLen, size_t stride);

	error remapMesh(
		const glm::vec3* positions,
		size_t vertexLen,
		size_t sizeOfVertex,
		const std::vector<uint32_t> indicies,
		const std::function<void* (size_t)>& resizeV,
		const std::function<uint32_t*(size_t)>& resizeI
	);

	error generateMeshlets(
		const glm::vec3* positions,
		size_t vertexLen, 
		size_t sizeOfVertex,
		const std::vector<uint32_t>& indicies,
		std::vector<meshlet>& mOut,
		std::vector<uint8_t>& pOut,
		std::vector<uint32_t>& iOut,
		size_t maxVert, size_t maxTriangles, float coneWieght,
		float errorLevel,
		size_t targetIndexCount
	);

	error generateLodLevel(
		const glm::vec3* positions,
		size_t vertexLen,
		size_t sizeOfVertex,
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