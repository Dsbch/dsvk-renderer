#include <pch.h>
#include "vertexBufferObject.h"
#include <glad/glad.h>

namespace engine
{

	int vertexBufferObject::mMaxAttributes;
	std::once_flag vertexBufferObject::mAttribOnceFlag;

	vertexBufferObject::vertexBufferObject() : mID(-1), mElementCount(-1), mAttribCount(0)
	{
		std::call_once(mAttribOnceFlag, glGetIntegerv, GL_MAX_VERTEX_ATTRIBS, &mMaxAttributes);

		glCreateVertexArrays(1, &mID);
	}

	vertexBufferObject::~vertexBufferObject()
	{
		glDeleteVertexArrays(1, &mID);
	}

	void vertexBufferObject::bind() const
	{
		glBindVertexArray(mID);
	}

	void vertexBufferObject::setElementBuffer(uint32_t elementCount, uint32_t elementBufferID)
	{
		mElementCount = elementCount;
		glVertexArrayElementBuffer(mID, elementBufferID);
	}

	uint32_t vertexBufferObject::getElementCount() const
	{
		return mElementCount;
	}

	engine::error vertexBufferObject::setAttribs(const attributesDescriber& describer)
	{
		auto info = describer.info();
		if (info.size() > mMaxAttributes)
		{
			return { "mMaxAttributes: {:d} but got {:d}", mMaxAttributes, info.size() };
		}

		for (const attributesDescriber::attributeInfo& i : info)
		{
			glEnableVertexArrayAttrib(mID, mAttribCount);
			glVertexArrayAttribBinding(mID, mAttribCount, mAttribCount);

			glVertexArrayVertexBuffer(mID, mAttribCount, i.bufferObjectID, 0, i.stride);
			glVertexArrayAttribFormat(mID, mAttribCount, i.count, i.type, i.needNormalization, i.offset);

			mAttribCount++;
		}

		return {};
	}
}