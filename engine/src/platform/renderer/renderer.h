#pragma once

#include <pch.h>
#include "base/context/context.h"
#include "shader.h"
#include "texture.h"
#include "vertex.h"
#include "platform/window/window.h"

namespace engine
{
	class renderer
	{
	public:
		struct renderCallIn
		{
			glm::mat4 viewProjection;
		};

		renderer(std::shared_ptr<context> ctx, std::shared_ptr<window> window) : mCtx(ctx), mWindow(window), mErr() {};
		virtual ~renderer() = default;
		virtual std::string getVersion() const = 0;
		virtual std::string getGpuName() const = 0;
		virtual error checkError() const = 0;
		virtual error changeViewPort(uint32_t width, uint32_t height) = 0;
		virtual error addToRender(model& m) = 0;
		virtual void removeFromRender(model& m) = 0;
		virtual error render(renderCallIn in) = 0;

		virtual withError<std::shared_ptr<shader>> makeShader(const std::vector<uint32_t>& src) = 0;
		virtual withError<std::shared_ptr<texture>> makeTexture(uint8_t* data, int width, int heigth, imageChannel channel) = 0;
	protected:
		error mErr;
		std::shared_ptr<context> mCtx;
		std::shared_ptr<window> mWindow;
	};

	std::shared_ptr<renderer> makeRenderer(std::shared_ptr<context> ctx, std::shared_ptr<window> window);
}
