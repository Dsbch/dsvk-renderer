#include "pch.h"
#include "win32/window.h"

namespace engine
{
	window::window(std::shared_ptr<context> ctx, const std::string& name, std::uint32_t width, std::uint32_t heigth, bool isFullscreen, bool showCursor)
		: mCtx(ctx), mName(name), mWidth(width), mHeight(heigth), mErr(), mIsFullscreen(isFullscreen), mShowCursor(showCursor)
	{
	}

	std::shared_ptr<window> makeWindow(std::shared_ptr<context> ctx, const std::string& name, std::uint32_t width, std::uint32_t heigth, bool isFullscreen, const std::string& applicationName, bool showCursor)
	{
#ifdef WIN32API
		return std::make_shared<winApiWindow>(ctx, name, width, heigth, isFullscreen, applicationName, showCursor);
#endif // WIN32API
	
		return std::shared_ptr<window>(nullptr);
	}
}