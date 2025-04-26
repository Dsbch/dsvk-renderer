#pragma once

#include <pch.h>

namespace engine {
	class error {
	private:
		std::string mValue;
	public:
		template <typename... T>
		error(const std::string& fmtStr, T&&... args);

		error() : mValue() {};
		error& operator=(const error& other);
		error& operator=(error&& other) noexcept;
		error(error&& e) noexcept;
		~error() = default;
		error(const error&) = default;

		std::string err() const;
		operator bool() const;
	};


	template<typename ...T>
	inline error::error(const std::string& fmtStr, T && ...args) : mValue(fmt::format(fmtStr, std::forward<T>(args)...)) {}
}
