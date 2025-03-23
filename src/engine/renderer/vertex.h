#pragma once

#include <pch.h>
#include <glm/vec3.hpp>
#include <glm/vec2.hpp>
#include "opengl/vertexBufferObject.h"
#include <glad/glad.h>

namespace engine {
	struct vertex {
		glm::vec3 position;
		glm::vec2 textureCoords;
		int textureIndex;
	};

	class vertexDescriber : public attributesDescriber {
	private:
		uint32_t mBufferObjectID;
	public:
		vertexDescriber(uint32_t vboID) : mBufferObjectID(vboID) {}
		std::vector<attributeInfo> info() const override
		{
			return {
				// position.
				{
					sizeof(vertex),
					mBufferObjectID,
					3,
					GL_FLOAT,
					offsetof(vertex, vertex::position),
					false,
				},
				// textureCoords.
				{
					sizeof(vertex),
					mBufferObjectID,
					2,
					GL_FLOAT,
					offsetof(vertex, vertex::textureCoords),
					false,
				},
				// textureIndex.
				{
					sizeof(vertex),
					mBufferObjectID,
					1,
					GL_INT,
					offsetof(vertex, vertex::textureIndex),
					false,
				},
			};
		}
	};
}

static_assert(std::is_pod_v<engine::vertex> == true);
