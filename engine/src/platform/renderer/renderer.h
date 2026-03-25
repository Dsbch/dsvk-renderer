#pragma once

#include <pch.h>
#include <entt/entt.hpp>
#include "base/context/context.h"
#include "shader.h"
#include "texture.h"
#include "vertex.h"
#include "platform/window/window.h"

namespace engine
{
	struct graphicsPreset
	{
		uint32_t msaa;
		uint32_t anisotropicFiltering;
	};

	class renderer
	{
	public:
		struct renderCallIn
		{
			float deltaTime;
			glm::mat4 debugCameraView;
			glm::mat4 debugCameraProjection;
			uint32_t useDebugCamera;
			glm::vec3 cameraPos;
			glm::vec3 cameraFront;
			glm::vec3 cameraUp;
			glm::mat4 view;
			glm::mat4 projection;
			frustum cameraFrustum;
		};

		renderer(std::shared_ptr<context> ctx, std::shared_ptr<window> window) : mCtx(ctx), mPreset(), mWindow(window), mErr() {};
		virtual ~renderer() = default;
		virtual graphicsPreset getGraphicsPreset() const;
		virtual void setGraphicsPreset(graphicsPreset);
		virtual std::string getVersion() const = 0;
		virtual std::string getGpuName() const = 0;
		virtual error checkError() const = 0;
		virtual error changeViewPort(uint32_t width, uint32_t height) = 0;
		virtual error addToRender(const model& m) = 0;
		virtual error updateInstance(const model& m) = 0;
		virtual void removeFromRender(const model& m) = 0;
		virtual error render(renderCallIn in) = 0;

		virtual withError<std::shared_ptr<const shader>> makeShader(const std::vector<uint32_t>& src) = 0;
		virtual withError<std::shared_ptr<const texture>> makeTexture(const image& img) = 0;
		virtual withError<std::shared_ptr<const texture>> makeTextureWithMips(const imageWithMipLevels& img) = 0;
	protected:
		error mErr;
		graphicsPreset mPreset;
		std::shared_ptr<context> mCtx;
		std::shared_ptr<window> mWindow;
	};

	std::shared_ptr<renderer> makeRenderer(std::shared_ptr<context> ctx, std::shared_ptr<window> window);
}
