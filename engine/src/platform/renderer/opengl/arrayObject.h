#pragma once

#include <pch.h>
#include "platform/renderer/arrayObject.h"

namespace engine
{
	class openglArrayObject : public arrayObject
	{
	public:
		openglArrayObject(size_t size, void* data);
		void updateData(uint32_t offset, uint32_t size, void* data) const;
		~openglArrayObject();
		size_t getSize() const;
		uint32_t getID() const;
	private:
		uint32_t mID;
	};

	class openglDynamicArrayObject : public dynamicArrayObject
	{
	public:
		openglDynamicArrayObject(size_t size, void* data);
		void updateData(size_t offset, size_t typeSize, size_t len, void* data);
		void* getPtr();
		~openglDynamicArrayObject();
		void setLoadedSize(size_t);
		size_t getSize() const;
		size_t getLoadedSize() const;
		uint32_t getID() const;
	private:
		uint32_t mID;
		void* mData;
	};
}