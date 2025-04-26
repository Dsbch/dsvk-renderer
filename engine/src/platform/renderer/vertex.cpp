#include <pch.h>
#include <glad/glad.h>
#include <glm/vec3.hpp>
#include <glm/vec2.hpp>
#include "vertex.h"

namespace engine {
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
					offsetof(vertex, position),
					false,
				},
				// textureCoords.
				{
					sizeof(vertex),
					mBufferObjectID,
					2,
					GL_FLOAT,
					offsetof(vertex, textureCoords),
					false,
				},
				// textureIndex.
				{
					sizeof(vertex),
					mBufferObjectID,
					1,
					GL_INT,
					offsetof(vertex, textureIndex),
					false,
				},
			};
		}
	}
}