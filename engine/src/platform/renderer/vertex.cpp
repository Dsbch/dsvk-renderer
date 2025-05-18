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
					sizeof(vertex::position)/sizeof(float),
					GL_FLOAT,
					offsetof(vertex, position),
					false,
					false,
				},
				// textureCoords.
				{
					sizeof(vertex),
					mBufferObjectID,
					sizeof(vertex::textureCoords) / sizeof(float),
					GL_FLOAT,
					offsetof(vertex, textureCoords),
					false,
					false,
				},
				// textureIndex.
				{
					sizeof(vertex),
					mBufferObjectID,
					sizeof(vertex::textureIndex) / sizeof(int),
					GL_INT,
					offsetof(vertex, textureIndex),
					false,
					false,
				},
			};
		}
	}

	std::vector<attributesDescriber::attributeInfo> instancedAttrDescriber::info() const
	{
		return {
			// Model matrix.
			{
				sizeof(instanceAttributes),
				mBufferObjectID,
				sizeof(instanceAttributes::modelMatrix) / sizeof(float),
				GL_FLOAT,
				offsetof(instanceAttributes, instanceAttributes::modelMatrix),
				false,
				true,
			},
		};
	}
}