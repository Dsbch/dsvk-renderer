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
		};

		struct sceneState
		{
			std::vector<model> addedEntities;
			std::vector<model> deletedEntities;
			std::vector<model> updateInstanceAttributes;
			std::vector<model> updateAnimations;
		};

		struct renderPackage
		{
		public:
			void setRenderParams(renderParams params);
			void addEntity(const model& m);
			void deleteEntity(const model& m);
			void updateInstanceAttributes(const model& m);
			void updateAnimations(const model& m);
			void swapSceneState();

			sceneState& getStateToRender();
			renderParams getRenderParams();
		private:
			renderParams mRenderCallParams;

			sceneState mPrevSceneState;
			sceneState mCurrentSceneState;

			co::mutex mMu;
		};

		renderer(std::shared_ptr<context> ctx, std::shared_ptr<window> window) : mCtx(ctx), mPreset(), mWindow(window), mPackage(std::make_shared<renderPackage>()), mErr() {};
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
