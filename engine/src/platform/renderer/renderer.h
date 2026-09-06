#pragma once

#include <pch.h>
#include <entt/entt.hpp>

#include "base/context/context.h"
#include "platform/window/window.h"

#include "shader.h"
#include "texture.h"
#include "vertex.h"

namespace engine
{
	struct sceneMetrics
	{
		uint32_t maxLodTriangles;
		uint32_t maxLodMeshlets;
		uint32_t entities;
	};

	struct globalMetrics
	{
		float fps;
		float deltaTime;
	};

	struct profilingInfo
	{
		std::set<uint32_t> inScene;

		sceneMetrics sceneInfo;
		globalMetrics globalInfo;
		std::map<std::string, float> passInfo;
	};

	struct graphicsPreset
	{
		uint32_t msaa;
		uint32_t anisotropicFiltering;
	};

	class renderer
	{
	public:
		struct renderParams
		{
			glm::mat4 debugCameraView;
			glm::mat4 debugCameraProjection;
			uint32_t useDebugCamera;
			glm::vec3 cameraPos;
			glm::vec3 cameraFront;
			glm::vec3 cameraUp;
			glm::mat4 view;
			glm::mat4 projection;
			frustum cameraFrustum;
			uint32_t width;
			uint32_t height;
			float verticalFov;
			float horizontalFov;
			float nearPlane;
			float farPlane;
		};

		struct sceneState
		{
			std::set<model> addedEntities;
			std::set<model> deletedEntities;
			std::set<model> updateInstanceAttributes;
			std::set<model> updateAnimations;
		};

		struct renderPackage
		{
		public:
			renderPackage(uint32_t framesInFlight);
			void setRenderParams(renderParams params);
			void addEntity(const model& m);
			void deleteEntity(const model& m);
			void updateInstanceAttributes(const model& m);
			void updateAnimations(const model& m);
			void swapSceneState();

			sceneState getStateToRender(uint32_t frame);
			renderParams getRenderParams();
		private:
			co::mutex mMu;
			renderParams mRenderCallParams;
			std::vector<sceneState> mCurrentSceneState;
		};

		renderer(std::shared_ptr<context> ctx, std::shared_ptr<window> window) : 
			mCtx(ctx), 
			mPreset(), 
			mWindow(window), 
			mPackage(std::make_shared<renderPackage>(ctx->config.inner.graphics.framesInFlight)), 
			mErr() 
		{};
		virtual ~renderer() = default;
		virtual std::string getVersion() const = 0;
		virtual std::string getGpuName() const = 0;
		virtual error checkError() const = 0;
	
		virtual error render() = 0;

		virtual withError<std::shared_ptr<const shader>> makeShader(const std::vector<uint32_t>& src) = 0;
		virtual withError<std::shared_ptr<const texture>> makeTexture(const image& img) = 0;
		virtual withError<std::shared_ptr<const texture>> makeTextureWithMips(const imageWithMipLevels& img) = 0;

		std::shared_ptr<renderPackage> getRenderPackage() const;
	protected:
		error mErr;
		graphicsPreset mPreset;
		std::shared_ptr<context> mCtx;
		std::shared_ptr<window> mWindow;

		std::shared_ptr<renderPackage> mPackage;
	};

	std::shared_ptr<renderer> makeRenderer(std::shared_ptr<context> ctx, std::shared_ptr<window> window);
}
