#pragma once

#include <pch.h>

namespace engine {
	template<class T>
	class arrayObject {
	protected:
		uint32_t mBufferSize;
	public:
		arrayObject(uint32_t size);
		virtual ~arrayObject();
		
		virtual core::error load(std::vector<T> &data) = 0;
	};
}
