#pragma once

#include <pch.h>
#include "platform/renderer/vertex.h"

struct cgltf_primitive;
struct cgltf_material;
struct cgltf_node;

namespace engine
{
	struct primitives
	{
		std::vector<vertex> vertecies;
		std::vector<animVertex> animVertecies;
		std::vector<uint32_t> indicies;
	};

	glm::mat4 getNodeWorldTransformMat4(const cgltf_node* node);
	glm::mat4 getNodeLocalTransformMat4(const cgltf_node* node);
	transform getNodeLocalTransform(const cgltf_node* node);

	primitives processPrimitive(const cgltf_primitive& prim, bool skinned, uint32_t localMaterialOffset = 0, uint32_t jointOffset = 0);
}