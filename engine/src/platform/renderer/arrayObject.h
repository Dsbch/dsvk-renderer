#pragma once

#include <pch.h>

namespace engine
{
	class arrayObject
	{
	public:
		arrayObject(size_t size, void* data) : mSize(size) {};
		virtual ~arrayObject() = default;
		virtual void updateData(uint32_t offset, uint32_t size, void* data) const = 0;
		virtual size_t getSize() const = 0;
		virtual uint32_t getID() const = 0;
	protected:
		size_t mSize;
	};

	// Persistantly mapped buffer.
	class dynamicArrayObject
	{
	public:
		dynamicArrayObject(size_t size, void* data) : mSize(size), mLoadedSize(0) {};
		virtual ~dynamicArrayObject() = default;
		virtual void updateData(size_t offset, size_t typeSize, size_t len, void* data) = 0;
		virtual void* getPtr() = 0;
		virtual void setLoadedSize(size_t) = 0;
		virtual size_t getSize() const = 0;
		virtual size_t getLoadedSize() const = 0;
		virtual uint32_t getID() const = 0;
	protected:
		size_t mSize;
		size_t mLoadedSize;
	};
}