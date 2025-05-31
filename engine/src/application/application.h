#pragma once

#include <pch.h>

#include "base/context/context.h"

namespace engine
{
	class window;
	class layerStack;
	class layer;

	class application
	{
	public:
		application();
		virtual ~application();

		void pushLayer(std::unique_ptr<layer>&&);
		void pushOverlay(std::unique_ptr<layer>&&);
		void run();
		error checkError();
	protected:
		error mErr;
		std::shared_ptr<context> mCtx;
		std::unique_ptr<window> mWindow;
		bool mRunning;
	private:
		std::unique_ptr<layerStack> mLayerStack;
		static application* app;

		error initApplication();
		error createWindow();
		error createLayerStack();
		void update(std::chrono::milliseconds& nextGameUpdate, std::chrono::milliseconds updateShift, uint32_t maxFrameSkip);
		void onRender(std::chrono::milliseconds& nextRender, std::chrono::milliseconds renderShift);
	};
}
