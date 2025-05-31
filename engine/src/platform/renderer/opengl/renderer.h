#pragma once

#include <pch.h>
#include "base/context/context.h"
#include "vertexArrayObject.h"
#include "platform/renderer/renderer.h"
#include "platform/renderer/shader.h"
#include "platform/renderer/texture.h"

namespace engine
{
	class openglRenderer : public renderer
	{
	public:
		openglRenderer(std::shared_ptr<context> ctx);
		std::string getVersion() const;
		error checkError() const;
		void changeViewPort(uint32_t width, uint32_t height) const;
		void clear() const;
		void render() const;
		void render(const shaderProgram* shader, texture* tex, const vertexArrayObject* vao) const;
		void render(const shaderProgram* shader, texture* tex, const vertexArrayObject* vao, uint32_t instanceCount) const;
	private:
		static std::once_flag mIsOpenglInitialized;
		static error mInitOpenglErr;
		static void initOpengl();
	};
}
