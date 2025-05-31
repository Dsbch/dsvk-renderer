#include <pch.h>
#include <glad/glad.h>
#include "arrayObject.h"

namespace engine
{
	openglArrayObject::openglArrayObject(size_t size, void* data) : arrayObject(size, data), mID(0)
	{
		glCreateBuffers(1, &mID);
		glNamedBufferStorage(mID, size, data, GL_DYNAMIC_STORAGE_BIT);
	}

	void openglArrayObject::updateData(uint32_t offset, uint32_t size, void* data) const
	{
		glNamedBufferSubData(mID, offset, size, data);
	}

	openglArrayObject ::~openglArrayObject()
	{
		glDeleteBuffers(1, &mID);
	}

	size_t openglArrayObject::getSize() const
	{
		return mSize;
	}

	uint32_t openglArrayObject::getID() const
	{
		return mID;
	}

	openglDynamicArrayObject::openglDynamicArrayObject(size_t size, void* data) : dynamicArrayObject(size, data), mID(0), mData(nullptr)
	{
		auto flags = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
		glCreateBuffers(1, &mID);
		glNamedBufferStorage(mID, size, data, flags);
		mData = glMapNamedBufferRange(mID, 0, mSize, flags);

		if (data)
		{
			mLoadedSize = mSize;
		}
	}

	void openglDynamicArrayObject::updateData(size_t offset, size_t typeSize, size_t len, void* data)
	{
		std::memcpy(static_cast<char*>(mData) + offset*typeSize, data, len * typeSize);
	}

	void* openglDynamicArrayObject::getPtr()
	{
		return mData;
	}

	openglDynamicArrayObject::~openglDynamicArrayObject()
	{
		glUnmapNamedBuffer(mID);
		glDeleteBuffers(1, &mID);
	}

	void openglDynamicArrayObject::setLoadedSize(size_t loadedSize)
	{
		mLoadedSize = loadedSize;
	}

	size_t openglDynamicArrayObject::getSize() const
	{
		return mSize;
	}

	size_t openglDynamicArrayObject::getLoadedSize() const
	{
		return mLoadedSize;
	}

	uint32_t openglDynamicArrayObject::getID() const
	{
		return mID;
	}

}