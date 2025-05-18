#include <pch.h>
#include "layerStack.h"

namespace engine
{
	void layerStack::onEvent(std::shared_ptr<baseEvent> e)
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

	void layerStack::onRender()
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

	void layerStack::onUpdate()
	{
		for (auto& l : mLayerStack)
		{
			l->onUpdate();
		}

		for (auto& l : mOverlayStack)
		{
			l->onUpdate();
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
	error layerStack::checkError() const
	{
		for (auto& l : mLayerStack)
		{
			if (auto err = l->checkError(); err)
				return err;
		}

		for (auto& l : mOverlayStack)
		{
			if (auto err = l->checkError(); err)
				return err;
		}

		return {};
	}
}