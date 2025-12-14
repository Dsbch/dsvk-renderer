//
// Basic instrumentation profiler by Cherno

// Usage: include this header file somewhere in your code (eg. precompiled header), and then use like:
//
// Instrumentor::Get().BeginSession("Session Name");        // Begin session 
// {
//     InstrumentationTimer timer("Profiled Scope Name");   // Place code like this in scopes you'd like to include in profiling
//     // Code
// }
// Instrumentor::Get().EndSession();                        // End Session
//
// You will probably want to macro-fy this, to switch on/off easily and use things like __FUNCSIG__ for the profile name.
//
#include <pch.h>
#include "profiling.h"

#ifdef DEBUG
namespace engine
{
	googleProfiler instrumentationTimer::mProfiler;

	void googleProfiler::writeProfile(const profileResult& result)
	{
		std::lock_guard<std::mutex> guard(mU);

		mJson["traceEvents"].push_back(
			{
				{"cat", "function"},
				{"dur", (result.end - result.start)},
				{"name", result.name},
				{"ph", "X"},
				{"pid", 0},
				{"tid", result.threadID},
				{"ts", result.start},
			}
			);
	}

	std::string googleProfiler::dump()
	{
		return mJson.dump(4);
	}

	instrumentationTimer::instrumentationTimer(const char* name)
		: mName(name)
	{
		mStartTimepoint = std::chrono::high_resolution_clock::now();
	}

	instrumentationTimer::~instrumentationTimer()
	{
		stop();
	}

	void instrumentationTimer::stop()
	{
		auto endTimepoint = std::chrono::high_resolution_clock::now();

		long long start = std::chrono::time_point_cast<std::chrono::microseconds>(mStartTimepoint).time_since_epoch().count();
		long long end = std::chrono::time_point_cast<std::chrono::microseconds>(endTimepoint).time_since_epoch().count();
		auto threadID = std::hash<std::thread::id>{}(std::this_thread::get_id());
		mProfiler.writeProfile({ mName, start, end, threadID });
	}

	void instrumentationTimer::dump(const std::string& fileName)
	{
		std::ofstream f{ fileName };
		f << mProfiler.dump();
		f.close();
	}

}
#endif // DEBUG
