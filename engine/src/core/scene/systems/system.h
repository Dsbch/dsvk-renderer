#pragma once

#include <pch.h>

#include "base/context/context.h"
#include "core/scene/scene.h"
#include "platform/renderer/renderer.h"

namespace engine
{
	// Core system has access to renderPackage and can affect rendering state.
	class coreSystem
	{
	public:
		coreSystem(std::shared_ptr<context> ctx, std::shared_ptr<renderer::renderPackage> renderPackage) : mCtx(ctx), mRenderPackage(renderPackage) {};
		virtual ~coreSystem() = default;
		virtual error checkError() = 0;

		virtual error onAttach(std::shared_ptr<registryHandle> registry) = 0;
		virtual void onDetach(std::shared_ptr<registryHandle> registry) = 0;
		
		virtual error onEvent(std::shared_ptr<registryHandle> registry, std::shared_ptr<baseEvent> e) = 0;

		virtual error onBeginUpdate(std::shared_ptr<registryHandle> registry) = 0;
		virtual error onUpdate(std::shared_ptr<registryHandle> registry, float deltaTime) = 0;
		virtual error onEndUpdate(std::shared_ptr<registryHandle> registry) = 0;
	protected:
		std::shared_ptr<context> mCtx;
		std::shared_ptr<renderer::renderPackage> mRenderPackage;
	};

	// User system only can have access to ECS.
	class userSystem
	{
	public:
		userSystem(std::shared_ptr<context> ctx) : mCtx(ctx) {};
		virtual ~userSystem() = default;
		virtual error checkError() = 0;

		virtual error onAttach(std::shared_ptr<registryHandle> registry) = 0;
		virtual void onDetach(std::shared_ptr<registryHandle> registry) = 0;

		virtual error onEvent(std::shared_ptr<registryHandle> registry, std::shared_ptr<baseEvent> e) = 0;

		virtual error onBeginUpdate(std::shared_ptr<registryHandle> registry) = 0;
		virtual error onUpdate(std::shared_ptr<registryHandle> registry, float deltaTime) = 0;
		virtual error onEndUpdate(std::shared_ptr<registryHandle> registry) = 0;
	protected:
		std::shared_ptr<context> mCtx;
	};
}