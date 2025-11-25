#include <pch.h>
#include "layerStack.h"

namespace engine
{
	error layerStack::onEvent(std::shared_ptr<baseEvent> e)
	{
		error err;

		for (auto begin = mOverlayStack.rbegin(); begin != mOverlayStack.rend(); begin++)
		{
			err = (*begin)->onEvent(e);
			if (err)
				return err;
		}

		for (auto begin = mLayerStack.rbegin(); begin != mLayerStack.rend(); begin++)
		{
			err = (*begin)->onEvent(e);
			if (err)
				return err;
		}

		return err;
	}

	error layerStack::onRender()
	{
		error err;

		for (auto& l : mLayerStack)
		{
			err = l->onRender();
			if (err)
				return err;
		}

		for (auto& l : mOverlayStack)
		{
			err = l->onRender();
			if (err)
				return err;
		}
	
		return err;
	}

	error layerStack::onUpdate()
	{
		error err;

		for (auto& l : mLayerStack)
		{
			err = l->onUpdate();
			if (err)
				return err;
		}

		for (auto& l : mOverlayStack)
		{
			err = l->onUpdate();
			if (err)
				return err;
		}

		return err;
	}

	void layerStack::pushLayer(std::unique_ptr<layer>&& l)
	{
		mLayerStack.push_back(std::move(l));
	}

	void layerStack::pushOverlay(std::unique_ptr<layer>&& l)
	{
		mOverlayStack.push_back(std::move(l));
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