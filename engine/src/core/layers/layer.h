#pragma once

#include <pch.h>
#include "base/context/context.h"

namespace engine
{
	class scene;

	class layer
	{
	public:
		layer(std::shared_ptr<context> ctx);
		virtual ~layer() = default;
		virtual bool onEvent(std::shared_ptr<baseEvent>) = 0;
		virtual void onRender() = 0;
		virtual void onUpdate() = 0;
		virtual error checkError() const = 0;
	protected:
		std::shared_ptr<context> mCtx;
	};

	class worldLayer : public layer {
	public:
		worldLayer(std::shared_ptr<context> ctx);
		~worldLayer();
		bool onEvent(std::shared_ptr<baseEvent> e);
		void onRender();
		void onUpdate();
		error checkError() const;
	private:
		std::shared_ptr<scene> mScene;
	};
}
