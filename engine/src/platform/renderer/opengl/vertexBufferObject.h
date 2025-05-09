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
		};

		attributesDescriber() = default;
		virtual ~attributesDescriber() = default;
		virtual std::vector<attributeInfo> info() const = 0;
	};

	class vertexArrayObject
	{
	private:
		uint32_t mID;
		size_t mAttribCount;
		size_t mElementCount;

		static int mMaxAttributes;
		static std::once_flag mAttribOnceFlag;
	public:
		vertexArrayObject();
		~vertexArrayObject();
		void bind() const;
		void unbind() const;
		void setElementBuffer(size_t elementCount, uint32_t elementBufferID);
		size_t getElementCount() const;

		engine::error setAttribs(const attributesDescriber&);
	};
}
