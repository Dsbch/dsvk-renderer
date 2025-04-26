#pragma once

#include <pch.h>
#include <nlohmann/json.hpp>

namespace engine
{
	template<class T>
	struct cfg {
	private:
		error mErr;
		T mCfg;
	public:
		cfg(const std::string& fileName = "config.json");
		error checkError() const;
		~cfg();
		T getCfg() const;
	};

	template<class T>
	inline cfg<T>::cfg(const std::string& fileName)
	{
		std::ifstream f(fileName, std::ifstream::in);
		if (!f)
		{
			mErr = error{ "fail on open file with name {}", fileName };
			return;
		}

		try
		{
			nlohmann::json parsed = nlohmann::json::parse(f);
			mCfg = parsed.get<T>();
		}
		catch (const std::exception& exc)
		{
			mErr = { exc.what() };
		}
	}

	template<class T>
	inline error cfg<T>::checkError() const
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
