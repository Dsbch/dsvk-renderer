#pragma once
#include <pch.h>
#include "core/layers/layer.h"
#include "core/events/events.h"

namespace engine
{
	class layerStack
	{
	private:
		std::vector<std::shared_ptr<layer>> mLayerStack;
		std::vector<std::shared_ptr<layer>> mOverlayStack;
	public:
		void dipsatchEvent(std::shared_ptr<baseEvent>);
		void render();
		void pushLayer(std::shared_ptr<layer> l);
		void pushOverlay(std::shared_ptr<layer> l);
		error checkError() const;
	};
}