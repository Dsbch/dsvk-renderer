#include <pch.h>
#include "vertex.h"
#ifdef OPENGL
#include <glad/glad.h>
#endif // OPENGL

namespace engine {
	std::vector<attributesDescriber::attributeInfo> vertexDescriber::info() const
	{
#ifdef OPENGL
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
				// normal.
				{
					sizeof(vertex),
					mBufferObjectID,
					sizeof(vertex::normal) / sizeof(float),
					GL_FLOAT,
					offsetof(vertex, normal),
					false,
					false,
				},
				// tangent.
				{
					sizeof(vertex),
					mBufferObjectID,
					sizeof(vertex::tangent) / sizeof(float),
					GL_FLOAT,
					offsetof(vertex, tangent),
					false,
					false,
				},
			};
		}
#endif // OPENGL

#ifdef VULKAN
		return {};
#endif // VULKAN

	}

	std::vector<attributesDescriber::attributeInfo> instancedAttrDescriber::info() const
	{
#ifdef OPENGL
		return {
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
		};
#endif // OPENGL

#ifdef VULKAN
		return {};
#endif // VULKAN
	}
}