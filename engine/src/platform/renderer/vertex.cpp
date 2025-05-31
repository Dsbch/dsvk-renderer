#include <pch.h>
#include "vertex.h"
#include <glad/glad.h>

namespace engine {
	std::vector<attributesDescriber::attributeInfo> vertexDescriber::info() const
	{
		{
			return {
				// position.
				{
					sizeof(vertex),
					mBufferObjectID,
					sizeof(vertex::position) / sizeof(float),
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
#ifdef OPENGL
			// Model matrix.
			// for some reason opengl can't have mat4 attribute, max size of vertex attribute is vec4 :(.
			{
				sizeof(instanceAttributes),
				mBufferObjectID,
				sizeof(instanceAttributes::modelMatrix) / sizeof(float) / 4,
				GL_FLOAT,
				offsetof(instanceAttributes, instanceAttributes::modelMatrix),
				false,
				true,
			},
			{
				sizeof(instanceAttributes),
				mBufferObjectID,
				sizeof(instanceAttributes::modelMatrix) / sizeof(float) / 4,
				GL_FLOAT,
				offsetof(instanceAttributes, instanceAttributes::modelMatrix) + sizeof(glm::vec4) * 1,
				false,
				true,
			},
			{
				sizeof(instanceAttributes),
				mBufferObjectID,
				sizeof(instanceAttributes::modelMatrix) / sizeof(float) / 4,
				GL_FLOAT,
				offsetof(instanceAttributes, instanceAttributes::modelMatrix) + sizeof(glm::vec4) * 2,
				false,
				true,
			},
			{
				sizeof(instanceAttributes),
				mBufferObjectID,
				sizeof(instanceAttributes::modelMatrix) / sizeof(float) / 4,
				GL_FLOAT,
				offsetof(instanceAttributes, instanceAttributes::modelMatrix) + sizeof(glm::vec4) * 3,
				false,
				true,
			},
#endif // OPENGL
		};
	}
}