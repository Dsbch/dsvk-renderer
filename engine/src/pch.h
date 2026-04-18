#pragma once

// STL.
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <functional>
#include <memory>
#include <atomic>
#include <inttypes.h>
#include <chrono>
#include <list>
#include <set>
#include <variant>
#include <filesystem>

// Concurrency.
#include <co/co.h>
#include <co/co/chan.h>
#include <co/co/wait_group.h>
#include <co/co/event.h>
#include <co/co/mutex.h>

// base.
#include <base/logger/logger.h>
#include <base/errors/errors.h>
#include <base/hash/hash.h>

template<typename F>
inline void measure(const std::string& name, F&& func)
{
	auto start = std::chrono::high_resolution_clock::now();
	func();
	auto end = std::chrono::high_resolution_clock::now();
	LOGINFO("{} took: {}", name, std::chrono::duration_cast<std::chrono::microseconds>(end - start).count());
}

template<typename F>
inline void goMain(F&& f)
{
	auto& scheds = co::scheds();

	scheds.front()->go(
		[f]()
		{
			try { f(); }
			catch (...) { LOGERROR("caught exception in goCatch"); }
		}
	);
}

template<typename F>
inline void goNotMain(F&& f)
{
	auto& scheds = co::scheds();
	static std::atomic<int> idx = 1;

	int schedCount = (int)scheds.size();
	if (schedCount <= 1)
	{
		go(
			[f]()
			{
				try { f(); }
				catch (...) { LOGERROR("caught exception in goCatch"); }
			}
		);

		return;
	}

	int i = 1 + (idx.fetch_add(1) % (schedCount - 1));
	scheds[i]->go(
		[f]()
		{
			try { f(); }
			catch (...) { LOGERROR("caught exception in goCatch"); }
		}
	);
}

template<typename F>
inline void goNotMainMeasure(const std::string& name, F&& f)
{
	goNotMain([name, f]() {measure(name, f); });
}