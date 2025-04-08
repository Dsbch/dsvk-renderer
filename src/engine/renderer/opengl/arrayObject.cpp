#include <pch.h>
#include "arrayObject.h"
#include <glad/glad.h>

engine::arrayObject::arrayObject(uint32_t size, void* data) : mSize(size), mID(-1)
{
	glCreateBuffers(1, &mID);
	glNamedBufferStorage(mID, size, data, GL_DYNAMIC_STORAGE_BIT);
}

void engine::arrayObject::updateData(uint32_t offset, uint32_t size, void* data) const
{
	glNamedBufferSubData(mID, offset, size, data);
}

engine::arrayObject ::~arrayObject()
{
	glDeleteBuffers(1, &mID);
}

uint32_t engine::arrayObject::getSize() const
{
	return mSize;
}

uint32_t engine::arrayObject::getID() const
{
	return mID;
}

engine::dynamicArrayObject::dynamicArrayObject(uint32_t size, void* data) : mSize(size), mID(-1), mData(nullptr)
{
	auto flags = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
	glCreateBuffers(1, &mID);
	glNamedBufferStorage(mID, size, data, flags);
	mData = glMapNamedBufferRange(mID, 0, mSize, flags);
}

engine::dynamicArrayObject::~dynamicArrayObject()
{
	glUnmapNamedBuffer(mID);
	glDeleteBuffers(1, &mID);
}

uint32_t engine::dynamicArrayObject::getSize() const
{
	return mSize;
}

uint32_t engine::dynamicArrayObject::getID() const
{
	return mID;
}
