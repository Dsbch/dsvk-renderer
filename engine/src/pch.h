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
#include <co/all.h>

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

inline co::wait_group gWG;

inline void waitDone()
{
	gWG.wait();
}

template<typename F>
inline void goCatch(F&& f)
{
	gWG.add(1);

	go(
		[f]()
		{
			defer(gWG.done());

			try { f(); }
			catch (...) { LOGERROR("caught exception in goCatch"); }
		}
	);
}

template<typename F>
inline void goCatchMeasure(const std::string& name, F&& f)
{
	goCatch([name, f]() {measure(name, f); });
}