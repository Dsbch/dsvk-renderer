#pragma once

#include <pch.h>

namespace engine {
	class error {
	private:
		std::string mValue;
	public:
		template <typename... T>
		error(const std::string& fmtStr, T&&... args);

		error();

		std::string err() const;
		operator bool() const;
	};

	template<typename ...T>
	inline error::error(const std::string& fmtStr, T&&... args)
		: mValue(fmt::format(fmt::runtime(fmtStr), std::forward<T>(args)...)) {}
}
