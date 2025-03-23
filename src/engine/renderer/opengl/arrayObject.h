#pragma once

#include <pch.h>

namespace engine {
	class arrayObject {
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
}