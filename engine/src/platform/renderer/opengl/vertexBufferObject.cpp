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

	void vertexArrayObject::unbind() const
	{
		glBindVertexArray(-1);
	}

	void vertexArrayObject::setElementBuffer(size_t elementCount, uint32_t elementBufferID)
	{
		mElementCount = elementCount;
		glVertexArrayElementBuffer(mID, elementBufferID);
	}

	size_t vertexArrayObject::getElementCount() const
	{
		return mElementCount;
	}

	engine::error vertexArrayObject::setAttribs(std::initializer_list<const attributesDescriber*> describer)
	{
		mAttribCount = 0;
		for (auto& d : describer)
		{
			if (!d)
				continue;

			auto info = d->info();
			if (info.size() + mAttribCount > mMaxAttributes)
			{
				return { "mMaxAttributes: {:d} but got {:d}", mMaxAttributes, info.size() + mAttribCount };
			}

			for (const attributesDescriber::attributeInfo& i : info)
			{
				glEnableVertexArrayAttrib(mID, GLuint(mAttribCount));
				glVertexArrayAttribBinding(mID, GLuint(mAttribCount), GLuint(mAttribCount));

				glVertexArrayVertexBuffer(mID, GLuint(mAttribCount), i.bufferObjectID, 0, i.stride);
				glVertexArrayAttribFormat(mID, GLuint(mAttribCount), i.count, i.type, i.needNormalization, i.offset);

				if (i.instanced)
					glVertexArrayBindingDivisor(mID, GLuint(mAttribCount), 1);

				mAttribCount++;
			}
		}

		return {};
	}
}