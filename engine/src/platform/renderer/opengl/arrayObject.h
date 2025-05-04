#pragma once

#include <pch.h>

namespace engine
{
	class arrayObject
	{
	private:
		uint32_t mSize;
		uint32_t mID;
	public:
		arrayObject(uint32_t size, void* data);
		void updateData(uint32_t offset, uint32_t size, void* data) const;
		~arrayObject();
		uint32_t getSize() const;
		uint32_t getID() const;
	};

	class dynamicArrayObject
	{
	private:
		size_t mSize;
		size_t mLoadedSize;
		uint32_t mID;
		void* mData;
	public:
		dynamicArrayObject(size_t size, void* data);
		template<class T>
		void updateData(size_t offset, size_t size, T* data);
		template<class T>
		T* getPtr();
		~dynamicArrayObject();
		void setLoadedSize(size_t);
		size_t getSize() const;
		size_t getLoadedSize() const;
		uint32_t getID() const;
	};

	template<class T>
	inline void dynamicArrayObject::updateData(size_t offset, size_t len, T* data)
	{
		std::memcpy(static_cast<T*>(mData) + offset, data, len * sizeof(T));
	}

	template<class T>
	inline T* dynamicArrayObject::getPtr()
	{
		return static_cast<T*>(mData);
	}
}