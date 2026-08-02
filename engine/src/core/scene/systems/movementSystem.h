#pragma once

#include <pch.h>

#include "system.h"
#include "core/scene/components.h"

namespace engine
{
	// Per-frame timing of the animation worker pool. Shared with the workers,
	// so it lives on the heap - coroutines must never point at a coroutine stack.
	struct animStats
	{
		std::atomic<long long> sumUs{ 0 };
		std::atomic<long long> maxUs{ 0 };
		std::atomic<long long> count{ 0 };

		void reset()
		{
			sumUs.store(0, std::memory_order_relaxed);
			maxUs.store(0, std::memory_order_relaxed);
			count.store(0, std::memory_order_relaxed);
		}

		void add(long long us)
		{
			sumUs.fetch_add(us, std::memory_order_relaxed);
			count.fetch_add(1, std::memory_order_relaxed);

			long long prev = maxUs.load(std::memory_order_relaxed);
			while (us > prev && !maxUs.compare_exchange_weak(prev, us, std::memory_order_relaxed))
			{
			}
		}
	};

	class movementSystem : public coreSystem
	{
	public:
		movementSystem(std::shared_ptr<context> ctx, std::shared_ptr<renderer::renderPackage> renderPackage);

		error onEvent(std::shared_ptr<registryHandle> registry, std::shared_ptr<baseEvent> e);

		error onAttach(std::shared_ptr<registryHandle> registry);
		void onDetach(std::shared_ptr<registryHandle> registry);

		error onBeginUpdate(std::shared_ptr<registryHandle> registry);
		error onUpdate(std::shared_ptr<registryHandle> registry, float deltaTime);
		error onEndUpdate(std::shared_ptr<registryHandle> registry);

		error checkError();
	private:
		error handleTransformedEntities(std::shared_ptr<registryHandle> registry);
		error handleAnimatedEntities(std::shared_ptr<registryHandle> registry, float deltaTime);

		struct animationWork
		{
			animationComponent anim;
			uidComponent uid;
			float deltaTime;
		};

		co::wait_group mWg;
		co::chan<animationWork> mAnimWorkChan;
		co::chan<model> mAnimResultChan;
	};
}