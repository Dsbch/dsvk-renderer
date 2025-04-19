#pragma once

#include <nlohmann/json.hpp>
#include "logger/logger.h"
#include "errors/errors.h"

namespace core
{
	template<class T>
	struct cfg {
	private:
		core::error mErr;
		T mCfg;
	public:
		cfg(const std::string& fileName = "config.json");
		core::error checkError() const;
		~cfg();
		T getCfg() const;
	};

	template<class T>
	inline cfg<T>::cfg(const std::string& fileName)
	{
		std::ifstream f(fileName, std::ifstream::in);
		if (!f)
		{
			mErr = core::error{ "fail on open file with name {}", fileName };
			return;
		}

		try
		{
			nlohmann::json parsed = nlohmann::json::parse(f);
			mCfg = parsed.get<config::main>();
		}
		catch (const std::exception& exc)
		{
			mErr = { exc.what() };
		}
	}

	template<class T>
	inline core::error cfg<T>::checkError() const
	{
		return mErr;
	}

	template<class T>
	inline cfg<T>::~cfg()
	{
	}

	template<class T>
	inline T cfg<T>::getCfg() const
	{
		return mCfg;
	}
}
