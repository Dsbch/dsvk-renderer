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
		uint32_t mSize;
		uint32_t mID;
		void* mData;
	public:
		dynamicArrayObject(uint32_t size, void* data);
		template<class T>
		void updateData(uint32_t offset, uint32_t size, T* data);
		~dynamicArrayObject();
		uint32_t getSize() const;
		uint32_t getID() const;
	};

	template<class T>
	inline void dynamicArrayObject::updateData(uint32_t offset, uint32_t size, T* data)
	{
		PROFILE_FUNC();

		std::memcpy(static_cast<T*>(mData) + offset, data, size * sizeof(T));
	}
}