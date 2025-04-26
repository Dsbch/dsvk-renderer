#pragma once

#include <pch.h>
#include "events/events.h"
#include "vertexBufferObject.h"
#include "context/context.h"

namespace engine
{
	class openglRenderer
	{
	private:
		static std::once_flag mIsOpenglInitialized;
		static error mInitOpenglErr;
		static void initOpengl();

		error mErr;
		context mCtx;
	public:
		openglRenderer(context ctx);
		std::string getVersion() const;
		error check() const;
		void changeViewPort(uint32_t width, uint32_t height) const;
		void render() const;
		void render(const vertexBufferObject& vao) const;
	};
}
