#pragma once

#include <pch.h>
#include "base/context/context.h"

namespace engine
{
	class scene;

	class layer
	{
	public:
		layer(context ctx);
		virtual ~layer() = default;
		virtual bool onEvent(std::shared_ptr<baseEvent>) = 0;
		virtual void onRender() = 0;
	protected:
		context mCtx;
	};

	class worldLayer : public layer {
	public:
		worldLayer(context ctx);
		~worldLayer();
		bool onEvent(std::shared_ptr<baseEvent> e);
		void onRender();
	private:
		std::shared_ptr<scene> mScene;
	};
}
