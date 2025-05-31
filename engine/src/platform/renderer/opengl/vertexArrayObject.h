#pragma once

#include <pch.h>
#include "platform/renderer/vertexArrayObject.h"

namespace engine
{
	class openglVertexArrayObject : public vertexArrayObject
	{
	public:
		openglVertexArrayObject();
		~openglVertexArrayObject();
		void bind() const;
		void unbind() const;
		void setElementBuffer(size_t elementCount, uint32_t elementBufferID);
		size_t getElementCount() const;
		engine::error setAttribs(std::initializer_list<const attributesDescriber*>);
	private:
		uint32_t mID;
		size_t mAttribCount;
		size_t mElementCount;

		static int mMaxAttributes;
		static std::once_flag mAttribOnceFlag;
	};
}
