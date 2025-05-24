#pragma once

#include <pch.h>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/mat3x3.hpp>
#include "opengl/vertexBufferObject.h"

namespace engine 
{
	struct vertex 
	{
		glm::vec3 position;
		glm::vec2 textureCoords;
		int textureIndex;
	};

	struct instanceAttributes
	{
		glm::mat4 modelMatrix;
	};

	class vertexDescriber : public attributesDescriber 
	{
	private:
		uint32_t mBufferObjectID;
	public:
		vertexDescriber(uint32_t vboID) : mBufferObjectID(vboID) {}
		std::vector<attributeInfo> info() const override;
	};

	class instancedAttrDescriber : public attributesDescriber
	{
	private:
		uint32_t mBufferObjectID;
	public:
		instancedAttrDescriber(uint32_t vboID) : mBufferObjectID(vboID) {}
		std::vector<attributeInfo> info() const override;
	};
}

static_assert(std::is_pod_v<engine::vertex> == true);
static_assert(std::is_pod_v<engine::instanceAttributes> == true);
