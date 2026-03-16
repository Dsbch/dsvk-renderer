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
		std::vector<uint32_t> indicies;
	};

	glm::mat4 getNodeWorldTransform(const cgltf_node* node);

	primitives processPrimitive(const cgltf_primitive& prim, const glm::mat4& transform, cgltf_material* materials);
}