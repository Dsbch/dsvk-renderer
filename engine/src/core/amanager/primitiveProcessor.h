#pragma once

#include <pch.h>
#include "platform/renderer/vertex.h"

struct cgltf_primitive;
struct cgltf_material;
struct cgltf_node;

namespace engine
{
	struct primitive
	{
		// Vertex attributes.
		// In pos fourth parameter is X texCoord.
		std::vector<glm::vec4> positions;
		// In normal fourth parameter is Y texCoord.
		std::vector<glm::vec4> normal;
		std::vector<glm::uvec4> jointIndices;
		std::vector<glm::vec4> weights;
		
		std::vector<uint32_t> indicies;
	};

	glm::mat4 getNodeWorldTransformMat4(const cgltf_node* node);
	glm::mat4 getNodeLocalTransformMat4(const cgltf_node* node);
	transform getNodeLocalTransform(const cgltf_node* node);

	withError<primitive> processPrimitive(const cgltf_primitive& prim, bool skinned, uint32_t jointOffset);
}