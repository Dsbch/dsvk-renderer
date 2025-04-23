#pragma once

#include <pch.h>
#include "events/events.h"
#include "vertexBufferObject.h"

namespace engine
{
	class openglRenderer
	{
	private:
		static std::once_flag mIsOpenglInitialized;
		static engine::error mInitOpenglErr;
		static void initOpengl();

		engine::error mErr;

	public:
		openglRenderer();
		std::string getVersion() const;
		engine::error check() const;
		void changeViewPort(uint32_t width, uint32_t height) const;
		void render() const;
		void render(const vertexBufferObject& vao) const;
	};
}
