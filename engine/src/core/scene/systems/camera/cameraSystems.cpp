#include <pch.h>

#include "cameraSystems.h"

namespace engine
{
	cameraSystems::cameraSystems(std::shared_ptr<context> ctx)
		: system(ctx), mCameraMovement(std::make_unique<cameraMovement>(ctx))
	{
	}

	error cameraSystems::checkError()
	{
		if (auto err = mCameraMovement->checkError(); err)
			return err;

		return {};
	}

	void cameraSystems::onUpdate(entt::registry& registry)
	{
		mCameraMovement->onUpdate(registry);
	}

	void cameraSystems::onRender(entt::registry& registry)
	{
		mCameraMovement->onRender(registry);
	}

	void cameraSystems::onEvent(entt::registry& registry, std::shared_ptr<baseEvent> e)
	{
		mCameraMovement->onEvent(registry, e);
	}
}