#include <pch.h>
#include "console.h"

core::windowConsole::windowConsole(const std::string& fileName, const std::string& mode, FILE* oldStream) :
	m_f(nullptr), m_fileName(fileName), m_mode(mode), m_oldStream(oldStream)
{
	AllocConsole();

	if (freopen_s(&m_f, m_fileName.c_str(), m_mode.c_str(), m_oldStream) != 0)
	{
		LOGERROR("Console freopen_s err");
		FreeConsole();
	}
}

core::windowConsole::~windowConsole()
{
	FreeConsole();
}

std::unique_ptr<core::console> core::createConsole()
{
#ifdef WIN32
	return std::make_unique<core::windowConsole>("CONOUT$", "w", stdout);
#endif // WIN32

	return std::make_unique<core::console>();
}
