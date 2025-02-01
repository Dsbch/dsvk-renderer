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
#include <json.h>

namespace core {
#ifdef DEBUG
	struct profileResult
	{
		std::string name;
		long long start, end;
		uint32_t threadID;
	};

	class googleProfiler
	{
	private:
		std::mutex mU;
		nlohmann::json mJson;
	public:
		googleProfiler() : mU() {}

		void writeProfile(const profileResult& result);

		std::string dump();
	};

	class instrumentationTimer
	{
	private:
		const char* mName;
		std::chrono::time_point<std::chrono::high_resolution_clock> mStartTimepoint;
		static googleProfiler mProfiler;
	public:
		instrumentationTimer(const char* name);

		~instrumentationTimer();

		void stop();

		static void dump(const std::string& fileName);
	};
}

#define PROFILE_FUNC() core::instrumentationTimer timer{__FUNCSIG__};
#define DUMP_PROFILING(fileName) core::instrumentationTimer::dump(fileName);
#else
#define PROFILE_FUNC()
#define DUMP_PROFILING(fileName)
#endif // DEBUG
