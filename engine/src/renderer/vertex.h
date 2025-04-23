#pragma once

#include <pch.h>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include "opengl/vertexBufferObject.h"

namespace engine 
{
	struct vertex 
	{
		glm::vec3 position;
		glm::vec2 textureCoords;
		int textureIndex;
	};

	class vertexDescriber : public attributesDescriber 
	{
	private:
		uint32_t mBufferObjectID;
	public:
		vertexDescriber(uint32_t vboID) : mBufferObjectID(vboID) {}
		std::vector<attributeInfo> info() const override;
	};
}

static_assert(std::is_pod_v<engine::vertex> == true);
