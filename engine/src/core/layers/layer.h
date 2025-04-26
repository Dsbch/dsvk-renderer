#pragma once

#include <pch.h>
#include <entt/entt.hpp>
#include "base/context/context.h"

namespace engine
{
	class layer
	{
	public:
		virtual ~layer() = default;
		virtual bool onEvent() = 0;
		virtual void onRender() = 0;
	protected:
		eventDispatcher mDispatcher;
	private:
		context mCtx;
	};

	class worldLayer : public layer {
	public:
		bool onEvent();
		void onRender();
	private:
		entt::registry mSceneRegistry;
	};
}
