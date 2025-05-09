#pragma once

#include <pch.h>

#include "base/context/context.h"

#ifdef WINAPI
int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow);
#else
int ::main(int argc, char** argv);
#endif

namespace engine
{
	class baseWindow;
	class layerStack;
	class layer;

	class application
	{
	public:
		application();
		virtual ~application();

		void pushLayer(std::shared_ptr<layer>);
		void pushOverlay(std::shared_ptr<layer>);
		void run();
		error checkError();
	protected:
		error mErr;
		cfg<main> mCfg;
		context mCtx;
		std::unique_ptr<baseWindow> mWindow;
		bool mRunning;
	private:
		std::unique_ptr<layerStack> mLayerStack;
		static application* app;

		error initApplication();
		error createWindow();
		error createLayerStack();
		void update(std::chrono::milliseconds& nextGameUpdate, std::chrono::milliseconds updateShift, uint32_t maxFrameSkip);
		void render(std::chrono::milliseconds& nextRender, std::chrono::milliseconds renderShift);
	};
}
