#pragma once
#include <pch.h>
#include "core/layers/layer.h"
#include "core/events/events.h"

namespace engine
{
	class layerStack
	{
	private:
		std::vector<std::unique_ptr<layer>> mLayerStack;
		std::vector<std::unique_ptr<layer>> mOverlayStack;
	public:
		void onEvent(std::shared_ptr<baseEvent>);
		void onRender();
		void onUpdate();
		void pushLayer(std::unique_ptr<layer>&& l);
		void pushOverlay(std::unique_ptr<layer>&& l);
		error checkError() const;
	};
}