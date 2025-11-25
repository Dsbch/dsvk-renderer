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

	error cameraSystems::onUpdate(entt::registry& registry)
	{
		return mCameraMovement->onUpdate(registry);
	}

	error cameraSystems::onRender(entt::registry& registry)
	{
		return mCameraMovement->onRender(registry);
	}

	error cameraSystems::onEvent(entt::registry& registry, std::shared_ptr<baseEvent> e)
	{
		return mCameraMovement->onEvent(registry, e);
	}
}