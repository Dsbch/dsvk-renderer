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

	renderer::renderPackage::renderPackage(uint32_t framesInFlight)
		:
		mCurrentSceneState(framesInFlight), mRenderCallParams({})
	{}

	void renderer::renderPackage::setRenderParams(renderParams params)
	{
		co::mutex_guard l{ mMu };

		mRenderCallParams = params;
	}

	void renderer::renderPackage::addEntity(const model& m)
	{
		co::mutex_guard l{ mMu };

		for (auto& v : mCurrentSceneState)
			v.addedEntities.insert(m);
	}

	void renderer::renderPackage::deleteEntity(const model& m)
	{
		co::mutex_guard l{ mMu };

		for (auto& v : mCurrentSceneState)
		{
			auto same = [&](const model& x) { return x.id == m.id; };
			std::erase_if(v.updateAnimations, same);
			std::erase_if(v.updateInstanceAttributes, same);
			std::erase_if(v.addedEntities, same);
			v.deletedEntities.insert(m);
		}
	}

	void renderer::renderPackage::updateInstanceAttributes(const model& m)
	{
		co::mutex_guard l{ mMu };

		for (auto& v : mCurrentSceneState)
		{
			v.updateInstanceAttributes.erase(m);
			v.updateInstanceAttributes.insert(m);
		}
	}

	void renderer::renderPackage::updateAnimations(const model& m)
	{
		co::mutex_guard l{ mMu };

		for (auto& v : mCurrentSceneState)
		{
			v.updateAnimations.erase(m);
			v.updateAnimations.insert(m);
		}
	}

	renderer::sceneState renderer::renderPackage::getStateToRender(uint32_t frame)
	{
		co::mutex_guard l{ mMu };

		using std::swap;

		sceneState sState = {};

		swap(sState, mCurrentSceneState[frame]);

		return sState;
	}

	renderer::renderParams renderer::renderPackage::getRenderParams()
	{
		co::mutex_guard l{ mMu };

		return mRenderCallParams;
	}
}
