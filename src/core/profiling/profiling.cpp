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
#pragma once

#include <pch.h>
#include "profiling.h"

#ifdef DEBUG
	GoogleProfiler InstrumentationTimer::mProfiler;

	void GoogleProfiler::WriteProfile(const ProfileResult& result)
	{
		std::lock_guard<std::mutex> guard(mU);

		mJson["traceEvents"].push_back(
			{
				{"cat", "function"},
				{"dur", (result.End - result.Start)},
				{"name", result.Name},
				{"ph", "X"},
				{"pid", 0},
				{"tid", result.ThreadID},
				{"ts", result.Start},
			}
			);
	}

	std::string GoogleProfiler::Dump()
	{
		return mJson.dump();
	}

	InstrumentationTimer::InstrumentationTimer(const char* name)
		: mName(name)
	{
		mStartTimepoint = std::chrono::high_resolution_clock::now();
	}

	InstrumentationTimer::~InstrumentationTimer()
	{
		Stop();
	}

	void InstrumentationTimer::Stop()
	{
		auto endTimepoint = std::chrono::high_resolution_clock::now();

		long long start = std::chrono::time_point_cast<std::chrono::microseconds>(mStartTimepoint).time_since_epoch().count();
		long long end = std::chrono::time_point_cast<std::chrono::microseconds>(endTimepoint).time_since_epoch().count();
		uint32_t threadID = std::hash<std::thread::id>{}(std::this_thread::get_id());
		mProfiler.WriteProfile({ mName, start, end, threadID });
	}

	void InstrumentationTimer::Dump(const std::string& fileName)
	{
		std::ofstream f{ fileName };
		f << mProfiler.Dump();
		f.close();
	}
#endif // DEBUG
