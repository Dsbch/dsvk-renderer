#pragma once

#include <pch.h>

namespace engineCore
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

	class vertexBufferObject
	{
	private:
		uint32_t mID;
		uint32_t mElementCount;
		uint32_t mAttribCount;

		static int mMaxAttributes;
		static std::once_flag mAttribOnceFlag;
	public:
		vertexBufferObject();
		~vertexBufferObject();
		void bind() const;
		void setElementBuffer(uint32_t elementCount, uint32_t elementBufferID);
		uint32_t getElementCount() const;

		engineCore::error setAttribs(const attributesDescriber&);
	};
}
