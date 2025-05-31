#pragma once

#include <pch.h>
#include "base/context/context.h"
#include "vertexArrayObject.h"
#include "shader.h"
#include "texture.h"

namespace engine
{
	class renderer
	{
	public:
		renderer(std::shared_ptr<context> ctx) : mCtx(ctx) {};
		virtual ~renderer() = default;
		virtual std::string getVersion() const = 0;
		virtual error checkError() const = 0;
		virtual void changeViewPort(uint32_t width, uint32_t height) const = 0;
		virtual void clear() const = 0;
		virtual void render() const = 0;
		virtual void render(const shaderProgram* shader, texture* tex, const vertexArrayObject* vao) const = 0;
		virtual void render(const shaderProgram* shader, texture* tex, const vertexArrayObject* vao, uint32_t instanceCount) const = 0;
	protected:
		error mErr;
		std::shared_ptr<context> mCtx;
	};
}
