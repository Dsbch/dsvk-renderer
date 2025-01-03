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

#ifdef DEBUG
	struct ProfileResult
	{
		std::string Name;
		long long Start, End;
		uint32_t ThreadID;
	};

	class GoogleProfiler
	{
	private:
		std::mutex mU;
		nlohmann::json mJson;
	public:
		GoogleProfiler() : mU() {}

		void WriteProfile(const ProfileResult& result);

		std::string Dump();
	};

	class InstrumentationTimer
	{
	private:
		const char* mName;
		std::chrono::time_point<std::chrono::high_resolution_clock> mStartTimepoint;
		static GoogleProfiler mProfiler;
	public:
		InstrumentationTimer(const char* name);

		~InstrumentationTimer();

		void Stop();

		static void Dump(const std::string& fileName);
	};

	#define PROFILE_FUNC() InstrumentationTimer timer{__FUNCSIG__};
	#define DUMP_PROFILING(fileName) InstrumentationTimer::Dump(fileName);
#else
	#define PROFILE_FUNC()
	#define DUMP_PROFILING(fileName)
#endif // DEBUG


