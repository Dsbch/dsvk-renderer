#pragma once

#include <pch.h>

namespace engine {
	class window {
	protected:
		std::string m_name;
	public:
		window(const std::string& name);
		virtual ~window();
	};
}