#pragma once

#include <pch.h>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/mat3x3.hpp>
#include "vertexArrayObject.h"

namespace engine 
{
	struct vertex 
	{
		glm::vec3 position;
		glm::vec2 textureCoords;
		glm::vec3 normal;
		glm::vec3 tangent;
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
		std::vector<attributeInfo> info() const;
	};

	class instancedAttrDescriber : public attributesDescriber
	{
	private:
		uint32_t mBufferObjectID;
	public:
		instancedAttrDescriber(uint32_t vboID) : mBufferObjectID(vboID) {}
		std::vector<attributeInfo> info() const;
	};

	struct drawElementsCommand
	{
		uint32_t vertexCount;	// amount of vertexes for the model.
		uint32_t instanceCount; // amount of instances to draw.
		uint32_t firstIndex;	// offset into index buffer.
		uint32_t baseVertex;	// offset into vertex buffer.
		uint32_t baseInstance;	// offset into perInstace buffer.
	};
}

static_assert(std::is_trivially_constructible_v<engine::vertex>&& std::is_standard_layout_v<engine::vertex>);
static_assert(std::is_trivially_constructible_v<engine::instanceAttributes>&& std::is_standard_layout_v<engine::instanceAttributes>);
static_assert(std::is_trivially_constructible_v<engine::drawElementsCommand>&& std::is_standard_layout_v<engine::drawElementsCommand>);
