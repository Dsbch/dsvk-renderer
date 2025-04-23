#include <pch.h>
#include "vertex.h"
#include <glad/glad.h>
#include <glm/vec3.hpp>
#include <glm/vec2.hpp>

namespace engineCore {
	std::vector<attributesDescriber::attributeInfo> vertexDescriber::info() const
	{
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
	}
}