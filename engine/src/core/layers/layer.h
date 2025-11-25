#pragma once

#include <pch.h>
#include "base/context/context.h"
#include "platform/window/window.h"

namespace engine
{
	class scene;

	class layer
	{
	public:
		layer(std::shared_ptr<context> ctx);
		virtual ~layer() = default;
		virtual error onEvent(std::shared_ptr<baseEvent>) = 0;
		virtual error onRender() = 0;
		virtual error onUpdate() = 0;
		virtual error checkError() const = 0;
	protected:
		std::shared_ptr<context> mCtx;
	};

	class worldLayer : public layer {
	public:
		worldLayer(std::shared_ptr<context> ctx, std::shared_ptr<window> wnd);
		~worldLayer();
		error onEvent(std::shared_ptr<baseEvent> e);
		error onRender();
		error onUpdate();
		error checkError() const;
	private:
		std::shared_ptr<scene> mScene;
	};
}
