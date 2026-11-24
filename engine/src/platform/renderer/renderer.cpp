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
		mCurrentSceneModelState(framesInFlight), mCurrentSceneLightState(framesInFlight), mRenderCallParams({})
	{}

	void renderer::renderPackage::setRenderParams(renderParams params)
	{
		co::mutex_guard l{ mMu };

		mRenderCallParams = params;
	}

	void renderer::renderPackage::addEntity(const model& m)
	{
		co::mutex_guard l{ mMu };

		for (auto& v : mCurrentSceneModelState)
			v.addedEntities.insert(m);
	}

	void renderer::renderPackage::deleteEntity(const model& m)
	{
		co::mutex_guard l{ mMu };

		for (auto& v : mCurrentSceneModelState)
		{
			auto same = [&](const model& x) { return x.id == m.id; };
			v.updateAnimations.erase(m.id);
			std::erase_if(v.updateInstanceAttributes, same);
			std::erase_if(v.addedEntities, same);
			v.deletedEntities.insert(m);
		}
	}

	void renderer::renderPackage::updateInstanceAttributes(const model& m)
	{
		co::mutex_guard l{ mMu };

		for (auto& v : mCurrentSceneModelState)
		{
			v.updateInstanceAttributes.erase(m);
			v.updateInstanceAttributes.insert(m);
		}
	}

	void renderer::renderPackage::updateAnimations(const std::pair<UID, std::vector<glm::mat4>>& toUpd)
	{
		co::mutex_guard l{ mMu };

		for (auto& v : mCurrentSceneModelState)
		{
			v.updateAnimations.erase(toUpd.first);
			v.updateAnimations[toUpd.first] = toUpd.second;
		}
	}

	void renderer::renderPackage::addLight(const light& m)
	{
		co::mutex_guard l{ mMu };

		for (auto& v : mCurrentSceneLightState)
			v.addedLights.insert(m);
	}

	void renderer::renderPackage::deleteLight(const light & m)
	{
		co::mutex_guard l{ mMu };

		for (auto& v : mCurrentSceneLightState)
		{
			auto same = [&](const light& x) { return x.id == m.id; };

			std::erase_if(v.updateLights, same);
			std::erase_if(v.addedLights, same);
			
			v.deletedLights.insert(m);
		}
	}

	void renderer::renderPackage::updateLight(const light & m)
	{
		co::mutex_guard l{ mMu };

		for (auto& v : mCurrentSceneLightState)
		{
			if (v.addedLights.contains(m))
			{
				v.addedLights.erase(m);
				v.addedLights.insert(m);
			}
			else
			{
				v.updateLights.erase(m);
				v.updateLights.insert(m);
			}
		}
	}

	renderer::sceneLightState renderer::renderPackage::getLightStateToRender(uint32_t frame)
	{
		co::mutex_guard l{ mMu };

		using std::swap;

		sceneLightState sState = {};

		swap(sState, mCurrentSceneLightState[frame]);

		return sState;
	}

	renderer::sceneModelState renderer::renderPackage::getModelStateToRender(uint32_t frame)
	{
		co::mutex_guard l{ mMu };

		using std::swap;

		sceneModelState sState = {};

		swap(sState, mCurrentSceneModelState[frame]);

		return sState;
	}

	renderer::renderParams renderer::renderPackage::getRenderParams()
	{
		co::mutex_guard l{ mMu };

		return mRenderCallParams;
	}
}
