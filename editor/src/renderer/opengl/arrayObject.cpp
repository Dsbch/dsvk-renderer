#include <pch.h>
#include "arrayObject.h"
#include <glad/glad.h>

namespace engine
{
	arrayObject::arrayObject(uint32_t size, void* data) : mSize(size), mID(-1)
	{
		glCreateBuffers(1, &mID);
		glNamedBufferStorage(mID, size, data, GL_DYNAMIC_STORAGE_BIT);
	}

	void arrayObject::updateData(uint32_t offset, uint32_t size, void* data) const
	{
		glNamedBufferSubData(mID, offset, size, data);
	}

	arrayObject ::~arrayObject()
	{
		glDeleteBuffers(1, &mID);
	}

	uint32_t arrayObject::getSize() const
	{
		return mSize;
	}

	uint32_t arrayObject::getID() const
	{
		return mID;
	}

	dynamicArrayObject::dynamicArrayObject(uint32_t size, void* data) : mSize(size), mID(-1), mData(nullptr)
	{
		auto flags = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
		glCreateBuffers(1, &mID);
		glNamedBufferStorage(mID, size, data, flags);
		mData = glMapNamedBufferRange(mID, 0, mSize, flags);
	}

	dynamicArrayObject::~dynamicArrayObject()
	{
		glUnmapNamedBuffer(mID);
		glDeleteBuffers(1, &mID);
	}

	uint32_t dynamicArrayObject::getSize() const
	{
		return mSize;
	}

	uint32_t dynamicArrayObject::getID() const
	{
		return mID;
	}

}