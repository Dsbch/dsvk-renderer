#include <pch.h>

#include "platform/renderer/vulkan/renderer.h"

namespace engine
{
	std::shared_ptr<renderer> makeRenderer(std::shared_ptr<context> ctx, std::shared_ptr<window> window)
	{
#ifdef VULKAN
		return std::make_shared<vulkanRenderer>(ctx, window);
#endif // VULKAN

		return std::shared_ptr<renderer>(nullptr);
	}

	std::shared_ptr<renderer::renderPackage> renderer::getRenderPackage() const
	{
		return mPackage;
	}

	void renderer::renderPackage::setRenderParams(renderParams params)
	{
		co::mutex_guard l{ mMu };

		mRenderCallParams = params;
	}

	void renderer::renderPackage::addEntity(const model& m)
	{
		co::mutex_guard l{ mMu };

		mCurrentSceneState.addedEntities.push_back(m);
	}

	void renderer::renderPackage::deleteEntity(const model& m)
	{
		co::mutex_guard l{ mMu };

		mCurrentSceneState.deletedEntities.push_back(m);
	}

	void renderer::renderPackage::updateInstanceAttributes(const model& m)
	{
		co::mutex_guard l{ mMu };

		mCurrentSceneState.updateInstanceAttributes.push_back(m);
	}

	void renderer::renderPackage::updateAnimations(const model& m)
	{
		co::mutex_guard l{ mMu };

		mCurrentSceneState.updateAnimations.push_back(m);
	}

	renderer::sceneState& renderer::renderPackage::getStateToRender()
	{
		co::mutex_guard l{ mMu };

		mPrevSceneState = {};

		std::swap(mPrevSceneState, mCurrentSceneState);

		return mPrevSceneState;
	}

	renderer::renderParams renderer::renderPackage::getRenderParams()
	{
		co::mutex_guard l{ mMu };

		return mRenderCallParams;
	}
}
