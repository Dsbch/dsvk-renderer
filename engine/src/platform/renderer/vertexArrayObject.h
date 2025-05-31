#pragma once

#include <pch.h>

namespace engine
{
	class attributesDescriber
	{
	public:
		struct attributeInfo
		{
			uint32_t stride;
			uint32_t bufferObjectID;
			uint32_t count;
			uint32_t type;
			uint32_t offset;
			bool needNormalization;
			bool instanced;
		};

		attributesDescriber() = default;
		virtual ~attributesDescriber() = default;
		virtual std::vector<attributeInfo> info() const = 0;
	};

	class vertexArrayObject
	{
	public:
		virtual ~vertexArrayObject() = default;
		virtual void bind() const = 0;
		virtual void unbind() const = 0;
		virtual void setElementBuffer(size_t elementCount, uint32_t elementBufferID) = 0;
		virtual size_t getElementCount() const = 0;
		virtual engine::error setAttribs(std::initializer_list<const attributesDescriber*>) = 0;
	};
}
