#include <pch.h>
#include <glad/glad.h>
#include "vertexBufferObject.h"

namespace engine
{

	int vertexArrayObject::mMaxAttributes;
	std::once_flag vertexArrayObject::mAttribOnceFlag;

	vertexArrayObject::vertexArrayObject() : mID(0), mElementCount(0), mAttribCount(0)
	{
		std::call_once(mAttribOnceFlag, glGetIntegerv, GL_MAX_VERTEX_ATTRIBS, &mMaxAttributes);

		glCreateVertexArrays(1, &mID);
	}

	vertexArrayObject::~vertexArrayObject()
	{
		glDeleteVertexArrays(1, &mID);
	}

	void vertexArrayObject::bind() const
	{
		glBindVertexArray(mID);
	}

	void vertexArrayObject::setElementBuffer(uint32_t elementCount, uint32_t elementBufferID)
	{
		mElementCount = elementCount;
		glVertexArrayElementBuffer(mID, elementBufferID);
	}

	uint32_t vertexArrayObject::getElementCount() const
	{
		return mElementCount;
	}

	engine::error vertexArrayObject::setAttribs(const attributesDescriber& describer)
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