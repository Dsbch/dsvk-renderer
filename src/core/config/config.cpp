#include <pch.h>
#include "config.h"

namespace config {
	void to_json(nlohmann::json& j, const gameLoop& p)
	{
		j = nlohmann::json{
			{"fps", p.fps},
			{"gups", p.gups},
			{"minimumFps", p.minimumFps},
		};
	}

	void from_json(const nlohmann::json& j, gameLoop& p)
	{
		j.at("fps").get_to(p.fps);
		j.at("gups").get_to(p.gups);
		j.at("minimumFps").get_to(p.minimumFps);
	}
	
	void to_json(nlohmann::json& j, const application& p)
	{
		j = nlohmann::json{
			{"name", p.name},
		};
	}

	void from_json(const nlohmann::json& j, application& p)
	{
		j.at("name").get_to(p.name);
	}

	void to_json(nlohmann::json& j, const logger& p)
	{
		j = nlohmann::json{
			{"file", p.file},
			{"pattern", p.pattern},
			{"level", p.level},
		};
	}

	void from_json(const nlohmann::json& j, logger& p)
	{
		j.at("file").get_to(p.file);
		j.at("pattern").get_to(p.pattern);
		j.at("level").get_to(p.level);
	}

	void to_json(nlohmann::json& j, const window& p)
	{
		j = nlohmann::json{
			{"width", p.width},
			{"height", p.height},
			{"isFullscreen", p.isFullscreen},
			{"name", p.name},
		};
	}

	void from_json(const nlohmann::json& j, window& p)
	{
		j.at("width").get_to(p.width);
		j.at("height").get_to(p.height);
		j.at("isFullscreen").get_to(p.isFullscreen);
		j.at("name").get_to(p.name);
	}

	void to_json(nlohmann::json& j, const main& p)
	{
		j = nlohmann::json{
			{"application", p.app},
			{"logger", p.log},
			{"window", p.wnd},
			{"gameLoop", p.gameLoop},
		};
	}

	void from_json(const nlohmann::json& j, main& p)
	{
		j.at("application").get_to(p.app);
		j.at("logger").get_to(p.log);
		j.at("window").get_to(p.wnd);
		j.at("gameLoop").get_to(p.gameLoop);
	}
}

core::cfg::cfg(const std::string& fileName)
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

core::error core::cfg::checkError() const
{
	return mErr;
}

core::cfg::~cfg()
{
}

config::main core::cfg::getCfg() const
{
	return mCfg;
}
