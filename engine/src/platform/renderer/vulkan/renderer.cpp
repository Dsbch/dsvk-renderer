#include <pch.h>
#define VMA_IMPLEMENTATION
#include "renderer.h"

namespace engine
{
	const uint32_t errCodeBufferOverFlow = 0;
	const uint32_t errCodeOutOfDateKHR = 1;

	vulkanRenderer::vulkanRenderer(std::shared_ptr<context> ctx, std::shared_ptr<window> window)
		:
		renderer(ctx, window),
		mWindowMinimized(false),
		mProfInfo(),
		mVulkanCtx(),
		mResourceManager()
	{
		mVulkanCtx = std::make_shared<vulkanContext>(ctx, window);

		mErr = mVulkanCtx->checkError();
		if (mErr)
			return;

		mPreset = mVulkanCtx->preset;

		mCtx->mAmanager->setMakeShaderFunc([&](const std::vector<uint32_t>& src) { return makeShader(src); });
		mCtx->mAmanager->setMakeTextureFunc([&](const image& img) { return makeTexture(img); });
		mCtx->mAmanager->setMakeTextureWithMipsFunc([&](const imageWithMipLevels& img) { return makeTextureWithMips(img); });

		mResourceManager = std::make_shared<resourceManager>();

		mResourceManager->init(ctx, mVulkanCtx);

		mErr = mResourceManager->build(window->getFbWidth(), window->getFbHeight());
		if (mErr)
			return;

		mErr = initPasses(window);
		if (mErr)
			return;
	}

	vulkanRenderer::~vulkanRenderer()
	{
		auto result = vkDeviceWaitIdle(mVulkanCtx->device);
		if (result != VK_SUCCESS)
			LOGERROR("~vulkanRenderer vkDeviceWaitIdle: {}", vkResultToStr(result));

		error err = mUiPass->destroy();
		if (err)
			LOGERROR("~vulkanRenderer mUiRenderer.destroy: {}", err.err());

		err = mMeshletPass->destroy();
		if (err)
			LOGERROR("~vulkanRenderer mMeshletRenderer.destroy {}", err.err());

		err = mLinePass->destroy();
		if (err)
			LOGERROR("~vulkanRenderer mLineRenderer.destroy {}", err.err());

		mResourceManager->destroy();

		mVulkanCtx->delQueue.flushDeletonQueue();

		mPackage.reset();
	}

	error vulkanRenderer::drawAsRaster(VkCommandBuffer cmd, renderer::renderParams in, uint32_t frameIndex)
	{
		error err = mVulkanCtx->profiler.beginTimeStamp(cmd, "drawOpaque", frameIndex);
		if (err)
			return err;

		// Begin a render pass connected to our draw image and depth buffer.
		err = drawOpaque(cmd, in, frameIndex);
		if (err)
			return err;

		mVulkanCtx->profiler.endTimestamp(cmd, "drawOpaque", frameIndex);

		err = mVulkanCtx->profiler.beginTimeStamp(cmd, "drawTransperent", frameIndex);

		err = drawTransperent(cmd, in, frameIndex);
		if (err)
			return err;

		mVulkanCtx->profiler.endTimestamp(cmd, "drawTransperent", frameIndex);

		// Transition to sample them as textures in composite pass.
		mResourceManager->transitionAccumImage(cmd, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		mResourceManager->transitionRevealImage(cmd, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		err = mVulkanCtx->profiler.beginTimeStamp(cmd, "compositeOpaqueAndTransperent", frameIndex);

		err = compositeOpaqueAndTransperent(cmd, in, frameIndex);
		if (err)
			return err;

		mVulkanCtx->profiler.endTimestamp(cmd, "compositeOpaqueAndTransperent", frameIndex);

		mResourceManager->transitionAccumImage(cmd, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		mResourceManager->transitionRevealImage(cmd, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

		return {};
	}

	error vulkanRenderer::drawAsVoxels(VkCommandBuffer cmd, renderer::renderParams in, uint32_t frameIndex)
	{
		setViewportAndSciccors(
			cmd,
			VkExtent3D{
				.width = mCtx->config.inner.graphics.clipMapResolution,
				.height = mCtx->config.inner.graphics.clipMapResolution,
				.depth = 1,
			}
			);

		error err = mVulkanCtx->profiler.beginTimeStamp(cmd, "voxelizeScene", frameIndex);
		if (err)
			return err;

		err = mVoxelPass->voxelizeScene(cmd, in, frameIndex);
		if (err)
			return err;

		mVulkanCtx->profiler.endTimestamp(cmd, "voxelizeScene", frameIndex);

		setViewportAndSciccors(cmd, mResourceManager->getColorAttachmentImage(false).img.extent);

		err = mVulkanCtx->profiler.beginTimeStamp(cmd, "visualizeVoxelScene", frameIndex);
		if (err)
			return err;

		err = mVoxelPass->visualizeVoxelScene(cmd, in, frameIndex);
		if (err)
			return err;

		mVulkanCtx->profiler.endTimestamp(cmd, "visualizeVoxelScene", frameIndex);

		return {};
	}

	error vulkanRenderer::initPasses(std::shared_ptr<window> window)
	{
		mMeshletPass = std::make_unique<meshletPass>();
		mLinePass = std::make_unique<linePass>();
		mUiPass = std::make_unique<uiPass>();
		mVoxelPass = std::make_unique<voxelPass>();

		error err = mMeshletPass->init(mCtx, mVulkanCtx, mResourceManager);
		if (err)
			return err;

		err = mLinePass->init(mCtx, mVulkanCtx, mResourceManager);
		if (err)
			return err;

		err = mUiPass->init(mCtx, mVulkanCtx, mResourceManager, window->getGLFWhandle());
		if (err)
			return err;

		err = mVoxelPass->init(mCtx, mVulkanCtx, mResourceManager);
		if (err)
			return err;

		return {};
	}

	error vulkanRenderer::drawOpaque(VkCommandBuffer cmd, renderer::renderParams in, uint32_t frameIndex)
	{
		error err = mLinePass->drawLines(
			cmd,
			frameIndex
		);
		if (err)
			return err;

		err = mMeshletPass->opaquePass(
			cmd,
			in,
			frameIndex
		);
		if (err)
			return err;

		return {};
	}

	error vulkanRenderer::drawTransperent(VkCommandBuffer cmd, renderer::renderParams in, uint32_t frameIndex)
	{
		return mMeshletPass->accumilationPass(cmd, in, frameIndex);
	}

	error vulkanRenderer::compositeOpaqueAndTransperent(VkCommandBuffer cmd, renderer::renderParams in, uint32_t frameIndex)
	{
		return mMeshletPass->compositePass(cmd, in, frameIndex);
	}

	error vulkanRenderer::drawUI(VkCommandBuffer cmd)
	{
		return mUiPass->drawUI(cmd, mProfInfo);
	}

	std::string vulkanRenderer::getVersion() const
	{
		VkPhysicalDeviceProperties props{};
		vkGetPhysicalDeviceProperties(mVulkanCtx->physicalDevice, &props);

		uint32_t apiVersion = props.apiVersion;
		uint32_t major = VK_VERSION_MAJOR(apiVersion);
		uint32_t minor = VK_VERSION_MINOR(apiVersion);
		uint32_t patch = VK_VERSION_PATCH(apiVersion);

		return fmt::format("VULKAN VERSION: {}.{}.{}", major, minor, patch);
	}

	std::string vulkanRenderer::getGpuName() const
	{
		VkPhysicalDeviceProperties props{};
		vkGetPhysicalDeviceProperties(mVulkanCtx->physicalDevice, &props);

		return props.deviceName;
	}

	error vulkanRenderer::checkError() const
	{
		return mErr;
	}

	error vulkanRenderer::changeViewPort(uint32_t width, uint32_t height)
	{
		if (width == 0 || height == 0)
		{
			mWindowMinimized = true;
			return {};
		}
		else
		{
			mWindowMinimized = false;
		}

		error err = mVulkanCtx->changeViewPort(width, height);
		if (err)
			return err;

		err = mResourceManager->changeViewPort(width, height);
		if (err)
			return err;

		mUiPass->updateViewPortDependantDescriptors();

		return {};
	}

	error vulkanRenderer::render()
	{
		error err = handleEvents();
		if (err)
			return err;

		// Render.
		static auto nextRender = std::chrono::duration_cast<std::chrono::nanoseconds>(mCtx->appTimer.getTimeSinceStart());
		static auto renderShift = std::chrono::nanoseconds(std::chrono::seconds(1)) / mCtx->config.inner.gameLoop.fps;

		static auto last = std::chrono::steady_clock::now();

		auto now = std::chrono::steady_clock::now();
		auto deltaTime = std::chrono::duration<float>(now - last).count();

		if (std::chrono::duration_cast<std::chrono::nanoseconds>(mCtx->appTimer.getTimeSinceStart()) >= nextRender)
		{
			if (mWindowMinimized)
				return {};

			auto waitResult = mVulkanCtx->sChain.waitOnRenderFence();
			if (waitResult)
				return waitResult.err();

			uint32_t frameIndex = mVulkanCtx->sChain.getCurrentFrameIndex();

			const renderer::sceneState& renderState = mPackage->getStateToRender(frameIndex);

			// Prepare to render, add/update entities.
			{
				err = addToRender(renderState.addedEntities, frameIndex);
				if (err)
					return err;

				err = updateAnimations(renderState.updateAnimations, frameIndex);
				if (err)
					return err;

				err = updateInstance(renderState.updateInstanceAttributes, frameIndex);
				if (err)
					return err;

				removeFromRender(renderState.deletedEntities, frameIndex);
			}

			renderer::renderParams params = mPackage->getRenderParams();

			err = mResourceManager->updateDescriptors(frameIndex);
			if (err)
				return err;

			// Update global UBO.
			err = mResourceManager->updatePerDrawBuffer(
				perDrawData{
					.debugViewProjection = params.debugCameraProjection * params.debugCameraView,
					.useDebugCamera = params.useDebugCamera,
					.cameraFront = params.cameraFront,
					.cameraPos = params.cameraPos,
					.cameraUp = params.cameraUp,
					.view = params.view,
					.projection = params.projection,
					.viewProjection = params.projection * params.view,
					.cameraFrustum = params.cameraFrustum,
					.deltaTime = deltaTime,
					.width = params.width,
					.height = params.height,
					.verticalFov = params.verticalFov,
					.horizontalFov = params.horizontalFov,
					.nearPlane = params.nearPlane,
					.farPlane = params.farPlane,
					.voxelParams = getVoxelSceneParams(),
				},
				frameIndex
				);
			if (err)
				return err;

			// Register all queued events from submit, get semaphores to wait upon before render.
			auto waitSema = mVulkanCtx->iSubmit.getCurrentSemaInUse();
			std::vector<VkSubmitInfo2> commands = mVulkanCtx->iSubmit.getSumbitedCommands();

			auto vkResult = vkQueueSubmit2(mVulkanCtx->graphicsQueue, uint32_t(commands.size()), commands.data(), nullptr);
			if (vkResult != VK_SUCCESS)
				return vkResultToStr(vkResult);

			updateProfInfo(deltaTime, frameIndex);

			// request image from the swapchain.
			// keep in mind that we use swapChain semaphore as signaling here.
			err = mVulkanCtx->sChain.acquireImageIndex();
			if (err)
			{
				if (err.is(errCodeOutOfDateKHR))
				{
					mCtx->mGameEventQueue->queueEvent(std::make_shared<windowFrameBufferResizeEvent>(mWindow->getFbWidth(), mWindow->getFbHeight()));

					return {};
				}

				return err;
			}

			auto resetResult = mVulkanCtx->sChain.resetRenderFence();
			if (resetResult)
				return resetResult.err();

			resetResult = mVulkanCtx->sChain.resetCommandBuffer();
			if (resetResult)
				return resetResult.err();

			//naming it cmd for shorter writing
			VkCommandBuffer cmd = mVulkanCtx->sChain.getCommandBuffer();

			//begin the command buffer recording. We will use this command buffer exactly once, so we want to let vulkan know that
			VkCommandBufferBeginInfo cmdBeginInfo = commandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

			// start recording.
			vkResult = vkBeginCommandBuffer(cmd, &cmdBeginInfo);
			if (vkResult != VK_SUCCESS)
				return vkResultToStr(vkResult);

			setViewportAndSciccors(cmd, mResourceManager->getColorAttachmentImage(false).img.extent);

			mVulkanCtx->profiler.reset(cmd, frameIndex);

			// Cross frame barriers.
			mResourceManager->transitionDepthImage(
				cmd,
				VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
				VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
				VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
				VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
				VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT
			);

			mResourceManager->transitionColorAttachmentImage(
				cmd,
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				VK_PIPELINE_STAGE_2_TRANSFER_BIT | VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
				VK_ACCESS_2_TRANSFER_READ_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
				VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
				VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT
			);

			mResourceManager->transitionAccumImage(
				cmd,
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT,
				VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT
			);

			mResourceManager->transitionRevealImage(
				cmd,
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT,
				VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT
			);

			mResourceManager->transitionHzbChainImages(
				cmd,
				VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL,
				VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
				VK_ACCESS_2_SHADER_READ_BIT,
				VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
				VK_ACCESS_2_SHADER_WRITE_BIT
			);

			//err = drawAsRaster(cmd, params, frameIndex);
			err = drawAsVoxels(cmd, params, frameIndex);
			if (err)
				return err;

			// Preapre images for UI render, revel and accum already transitioned to needed layoyut.
			mResourceManager->transitionDepthImage(cmd, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
			mResourceManager->transitionHzbChainImages(cmd, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
			mResourceManager->transitionAccumImage(cmd, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
			mResourceManager->transitionRevealImage(cmd, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

			err = mVulkanCtx->profiler.beginTimeStamp(cmd, "drawUI", frameIndex);

			err = drawUI(cmd);
			if (err)
				return err;

			mVulkanCtx->profiler.endTimestamp(cmd, "drawUI", frameIndex);

			// Prepare for next frame.
			mResourceManager->transitionAccumImage(cmd, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
			mResourceManager->transitionRevealImage(cmd, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
			mResourceManager->transitionHzbChainImages(cmd, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL);
			mResourceManager->transitionDepthImage(cmd, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

			//transition the draw image and the swapchain image into their correct transfer layouts
			mResourceManager->transitionColorAttachmentImage(cmd, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

			mVulkanCtx->sChain.transitionCurrentSwapChainImage(
				cmd,
				VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
			);

			// copy from the draw image into the swapchain
			copyImageToImage(
				cmd,
				mResourceManager->getColorAttachmentImage(mVulkanCtx->preset.msaa > 1).img.image,
				mVulkanCtx->sChain.getCurrentSwapChainImage(),
				mResourceManager->getColorAttachmentImage(false).img.extent,
				mVulkanCtx->sChain.getSwapChainExtent()
			);

			// Transition image back to it's format.
			mResourceManager->transitionColorAttachmentImage(cmd, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

			// set swapchain image layout to Attachment Optimal so we can draw it
			mVulkanCtx->sChain.transitionCurrentSwapChainImage(
				cmd,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
			);

			// set swapchain image layout to Present so we can draw it
			mVulkanCtx->sChain.transitionCurrentSwapChainImage(
				cmd,
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
			);

			//finalize the command buffer (we can no longer add commands, but it can now be executed)
			vkResult = vkEndCommandBuffer(cmd);
			if (vkResult != VK_SUCCESS)
				return vkResultToStr(vkResult);

			// Prepare the submission to the queue. 
			//	we want to wait on the _presentSemaphore and all semaphores that were created during resource creating, 
			//  _presentSemaphore semaphore is signaled when the swapchain is ready.
			// Remember when we ask GPU for image from swap chain we provide that semaphore to signal.
			// We will signal the _renderSemaphore, to signal that rendering has finished.
			VkCommandBufferSubmitInfo cmdinfo = commandBufferSubmitInfo(cmd);

			std::vector<VkSemaphoreSubmitInfo> waitInfo{};
			std::vector<VkSemaphoreSubmitInfo> signalInfo{};

			waitInfo.push_back(semaphoreSubmitInfo(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR, mVulkanCtx->sChain.getSwapchainSemaphore()));
			signalInfo.push_back(semaphoreSubmitInfo(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, mVulkanCtx->sChain.getRenderSemaphore()));

			for (auto& sema : waitSema)
				waitInfo.push_back(semaphoreSubmitInfo(VK_PIPELINE_STAGE_2_TASK_SHADER_BIT_EXT | VK_PIPELINE_STAGE_2_MESH_SHADER_BIT_EXT, sema));

			VkSubmitInfo2 submit = submitInfo(&cmdinfo, signalInfo, waitInfo);

			// submit command buffer to the queue and execute it.
			// _renderFence will now block until the graphic commands finish execution.
			vkResult = vkQueueSubmit2(mVulkanCtx->graphicsQueue, 1, &submit, mVulkanCtx->sChain.getRenderFence());
			if (vkResult != VK_SUCCESS)
				return vkResultToStr(vkResult);

			// Delete all submitted commands and semaphores.
			mVulkanCtx->iSubmit.deleteSemaInUse(waitSema.size());
			mVulkanCtx->iSubmit.deleteSubmitedCommands(commands.size());

			// prepare present.
			// this will put the image we just rendered to into the visible window.
			// we want to wait on the _renderSemaphore for that, 
			// as its necessary that drawing commands have finished before the image is displayed to the user.
			auto presentErr = mVulkanCtx->sChain.present(mVulkanCtx->graphicsQueue);
			if (presentErr)
				return presentErr;

			last = now;

			nextRender += renderShift;

			if (std::chrono::duration_cast<std::chrono::nanoseconds>(mCtx->appTimer.getTimeSinceStart()) >= nextRender)
				nextRender = std::chrono::duration_cast<std::chrono::nanoseconds>(mCtx->appTimer.getTimeSinceStart());
		}

		return {};
	}

	withError<std::shared_ptr<const shader>> vulkanRenderer::makeShader(const std::vector<uint32_t>& src)
	{
		std::shared_ptr<const shader> vkShader = std::make_shared<const vulkanShader>(mVulkanCtx->device, src);
		if (vkShader->checkError())
			return vkShader->checkError();

		return vkShader;
	}

	withError<std::shared_ptr<const texture>> vulkanRenderer::makeTexture(const image& img)
	{
		std::shared_ptr<const texture> vkTexture = std::make_shared<const vulkanTexture>(mVulkanCtx->device, mVulkanCtx->allocator, mVulkanCtx->iSubmit, img);
		if (vkTexture->checkError())
			return vkTexture->checkError();

		return vkTexture;
	}

	withError<std::shared_ptr<const texture>> vulkanRenderer::makeTextureWithMips(const imageWithMipLevels& img)
	{
		std::shared_ptr<const texture> vkTexture = std::make_shared<const vulkanTexture>(mVulkanCtx->device, mVulkanCtx->allocator, mVulkanCtx->iSubmit, img);
		if (vkTexture->checkError())
			return vkTexture->checkError();

		return vkTexture;
	}

	void vulkanRenderer::updateProfInfo(float deltaTime, uint32_t frameIndex)
	{
		static uint32_t frames = 0;
		static auto lastCall = std::chrono::steady_clock::now();

		frames++;
		auto now = std::chrono::steady_clock::now();
		float elapsed = std::chrono::duration<float>(now - lastCall).count();

		if (elapsed >= 1.0f)
		{
			mProfInfo.globalInfo.fps = frames / elapsed;
			frames = 0;
			lastCall = now;
		}

		mProfInfo.globalInfo.deltaTime = deltaTime;

		mProfInfo.passInfo = mVulkanCtx->profiler.getAllSlots(frameIndex);
	}

	void vulkanRenderer::registerSceneMetrics(const model& m, bool isDeleted)
	{
		if (isDeleted)
		{
			if (!mProfInfo.inScene.contains(m.id))
				return;

			for (auto& mesh : *m.meshData.get())
			{
				mProfInfo.sceneInfo.maxLodMeshlets -= mesh.meshlets.second;

				for (uint32_t i = 0; i < mesh.meshlets.second; i++)
					mProfInfo.sceneInfo.maxLodTriangles -= mesh.meshlets.data[i].triangleCount;
			}

			mProfInfo.sceneInfo.entities--;

			mProfInfo.inScene.erase(m.id);
		}
		else
		{
			if (mProfInfo.inScene.contains(m.id))
				return;

			for (auto& mesh : *m.meshData.get())
			{
				mProfInfo.sceneInfo.maxLodMeshlets += mesh.meshlets.second;

				for (uint32_t i = 0; i < mesh.meshlets.second; i++)
					mProfInfo.sceneInfo.maxLodTriangles += mesh.meshlets.data[i].triangleCount;
			}

			mProfInfo.sceneInfo.entities++;

			mProfInfo.inScene.insert(m.id);
		}
	}

	void vulkanRenderer::visualizeNormals(const model& m)
	{
		for (int y = 0; y < m.meshData->size(); y++)
		{
			auto perMeshAttr = m.perMeshData->operator[](y);
			auto mesh = m.meshData->operator[](y);

			for (int i = 0; i < mesh.positions.size(); i++)
			{
				glm::vec3 pos = mesh.positions[i];
				glm::vec3 normal = mesh.normal[i];

				pos = perMeshAttr.meshGlobalTransform * glm::vec4{ pos, 1.0f };
				pos = glm::vec3(
					m.instanceAttributes.modelTransform.translation + m.instanceAttributes.modelTransform.rotation * m.instanceAttributes.modelTransform.scale * pos
				);

				normal = glm::transpose(glm::inverse(glm::mat3(perMeshAttr.meshGlobalTransform))) * normal;
				normal = glm::normalize(m.instanceAttributes.modelTransform.rotation * normal);

				mResourceManager->addLine(
					line{ .p1 = pos, .p2 = pos + normal / 10.0f }
				);
			}
		}
	}

	void vulkanRenderer::visualizeAABB(const model& m)
	{
		const aabb box = m.calculateWorldSpaceAABB();

		visualizeAABB(box);
	}

	void vulkanRenderer::visualizeAABB(const aabb& box)
	{

		const float cosA = glm::dot(glm::normalize(box.min - box.max), glm::vec3{ 0.0f, 0.0f, -1.0f });
		const float cosB = glm::dot(glm::normalize(box.min - box.max), glm::vec3{ -1.0f, 0.0f, 0.0f });
		const float cosY = glm::dot(glm::normalize(box.min - box.max), glm::vec3{ 0.0f, -1.0f, 0.0f });

		const float width = glm::length(box.min - box.max) * cosB;
		const float length = glm::length(box.min - box.max) * cosA;
		const float height = glm::length(box.min - box.max) * cosY;

		const std::array<glm::vec3, 24> lines{
			box.max,
			box.max + glm::vec3{ -1.0f, 0.0f, 0.0f } * width,

			box.max,
			box.max + glm::vec3{ 0.0f, -1.0f, 0.0f } * height,

			box.max,
			box.max + glm::vec3{ 0.0f, 0.0f, -1.0f } * length,

			box.min,
			box.min + glm::vec3{ 1.0f, 0.0f, 0.0f } * width,

			box.min,
			box.min + glm::vec3{ 0.0f, 1.0f, 0.0f } * height,

			box.min,
			box.min + glm::vec3{ 0.0f, 0.0f, 1.0f } * length,

			box.min + glm::vec3{ 0.0f, 0.0f, 1.0f } * length,
			box.max + glm::vec3{ 0.0f, -1.0f, 0.0f } * height,

			box.max + glm::vec3{ 0.0f, 0.0f, -1.0f } * length,
			box.min + glm::vec3{ 0.0f, 1.0f, 0.0f } * height,

			box.min + glm::vec3{ 0.0f, 0.0f, 1.0f } * length,
			box.max + glm::vec3{ -1.0f, 0.0f, 0.0f } * width,

			box.max + glm::vec3{ -1.0f, 0.0f, 0.0f } * width,
			box.min + glm::vec3{ 0.0f, 1.0f, 0.0f } * height,

			box.max + glm::vec3{ 0.0f, -1.0f, 0.0f } * height,
			box.min + glm::vec3{ 1.0f, 0.0f, 0.0f } * width,

			box.max + glm::vec3{ 0.0f, 0.0f, -1.0f } * length,
			box.min + glm::vec3{ 1.0f, 0.0f, 0.0f } * width,
		};

		for (size_t i = 1; i < lines.size(); i += 2)
			mResourceManager->addLine(
				line{ .p1 = lines[i], .p2 = lines[i - 1] }
			);
	}

	error vulkanRenderer::handleEvents()
	{
		error err = {};

		auto events = mCtx->mRenderEventQueue->purgeAndGet();

		while (!events.empty())
		{
			auto event = events.front();
			events.pop();

			if (event->getEventType() == eventType::frameBufferReisze)
			{
				auto resizeEvent = static_cast<windowFrameBufferResizeEvent*>(event.get());

				err = changeViewPort(resizeEvent->getWidth(), resizeEvent->getHeight());
				if (err)
					return err;
			}
		}

		return err;
	}

	error vulkanRenderer::addToRender(const std::set<model>& addedEntities, uint32_t frameIndex)
	{
		error err = {};
		for (auto& m : addedEntities)
		{
			mSceneAABB[m.id] = m.calculateWorldSpaceAABB();

			registerSceneMetrics(m);

			err = mMeshletPass->createPipeline(m);
			if (err)
				return err;

			err = mResourceManager->addToRender(m, frameIndex);
			if (err)
				return err;
		}

		return err;
	}

	error vulkanRenderer::updateInstance(const std::set<model>& updatedEntities, uint32_t frameIndex)
	{
		error err = {};
		for (auto& m : updatedEntities)
		{
			err = mResourceManager->updateInstance(m, frameIndex);
			if (err)
				return err;
		}

		return err;
	}

	error vulkanRenderer::updateAnimations(const std::set<model>& animationUpdatedEntities, uint32_t frameIndex)
	{
		error err = {};
		for (auto& m : animationUpdatedEntities)
		{
			err = mResourceManager->updateAnimations(m, frameIndex);
			if (err)
				return err;
		}

		return err;
	}

	void vulkanRenderer::removeFromRender(const std::set<model>& deletedEntities, uint32_t frameIndex)
	{
		for (auto& m : deletedEntities)
		{
			registerSceneMetrics(m, true);

			mResourceManager->removeFromRender(m, frameIndex);
		}
	}

	aabb vulkanRenderer::getSceneBoundingBox() const
	{
		aabb result{
			.min = glm::vec3{std::numeric_limits<float>::max()},
			.max = glm::vec3{std::numeric_limits<float>::lowest()},
		};

		if (mSceneAABB.size() == 0)
			return aabb{};

		for (auto& [_, v] : mSceneAABB)
		{
			result.max = glm::max(result.max, v.max);
			result.min = glm::min(result.min, v.min);
		}

		return result;
	}

	voxelDrawParams vulkanRenderer::getVoxelSceneParams() const
	{
		aabb box = getSceneBoundingBox();

		glm::mat4 view = glm::mat4{ 1.0f };

		const float halfExtent = float(mCtx->config.inner.graphics.voxelSceneExtent) * 0.5f;

		glm::mat4 proj = glm::ortho(
			-halfExtent, halfExtent,
			-halfExtent, halfExtent,
			-halfExtent, halfExtent);

		voxelDrawParams result{
			.clipMapResolution = mCtx->config.inner.graphics.clipMapResolution,
			.voxelSceneExtent = mCtx->config.inner.graphics.voxelSceneExtent,
			.viewVoxel = view,
			.projectionVoxel = proj,
			.viewProjectionVoxel = proj * view,
		};

		return result;
	}
}
