#include <pch.h>
#include "layerStack.h"

namespace engine
{
	void layerStack::dipsatchEvent(std::shared_ptr<baseEvent> e)
	{
		for (auto begin = mOverlayStack.rbegin(); begin != mOverlayStack.rend(); begin++)
		{
			if ((*begin)->onEvent(e))
				return;
		}

		for (auto begin = mLayerStack.rbegin(); begin != mLayerStack.rend(); begin++)
		{
			if ((*begin)->onEvent(e))
				return;
		}
	}

	void layerStack::render()
	{
		for (auto& l : mLayerStack)
		{
			l->onRender();
		}

		for (auto& l : mOverlayStack)
		{
			l->onRender();
		}
	}

	void layerStack::pushLayer(std::shared_ptr<layer> l)
	{
		mLayerStack.push_back(l);
	}

	void layerStack::pushOverlay(std::shared_ptr<layer> l)
	{
		mOverlayStack.push_back(l);
	}
}