#include <pch.h>
#include "vertexBufferObject.h"
#include <glad/glad.h>

int engine::vertexBufferObject::mMaxAttributes;
std::once_flag engine::vertexBufferObject::mAttribOnceFlag;

engine::vertexBufferObject::vertexBufferObject() : mID(-1), mElementCount(-1), mAttribCount(0)
{
	std::call_once(mAttribOnceFlag, glGetIntegerv, GL_MAX_VERTEX_ATTRIBS, &mMaxAttributes);

	glCreateVertexArrays(1, &mID);
}

engine::vertexBufferObject::~vertexBufferObject()
{
	glDeleteVertexArrays(1, &mID);
}

void engine::vertexBufferObject::bind() const
{
	glBindVertexArray(mID);
}

void engine::vertexBufferObject::setElementBuffer(uint32_t elementCount, uint32_t elementBufferID)
{
	mElementCount = elementCount;
	glVertexArrayElementBuffer(mID, elementBufferID);
}

uint32_t engine::vertexBufferObject::getElementCount() const
{
	return mElementCount;
}

core::error engine::vertexBufferObject::setAttribs(const attributesDescriber& describer)
{
	auto info = describer.info();
	if (info.size() > mMaxAttributes)
	{
		return { "mMaxAttributes: {:d} but got {:d}", mMaxAttributes, info.size() };
	}

	for (const engine::attributesDescriber::attributeInfo& i : info)
	{
		glEnableVertexArrayAttrib(mID, mAttribCount);
		glVertexArrayAttribBinding(mID, mAttribCount, mAttribCount);

		glVertexArrayVertexBuffer(mID, mAttribCount, i.bufferObjectID, 0, i.stride);
		glVertexArrayAttribFormat(mID, mAttribCount, i.count, i.type, i.needNormalization, i.offset);

		mAttribCount++;
	}

	return {};
}