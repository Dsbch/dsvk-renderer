#pragma once

#include <pch.h>
#include "vertexBufferObject.h"
#include "shader.h"
#include "base/context/context.h"
#include "core/events/events.h"

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
		error checkError() const;
		void changeViewPort(uint32_t width, uint32_t height) const;
		void render() const;
		void render(const shaderProgram& shader, const vertexArrayObject& vao) const;
	};
}
