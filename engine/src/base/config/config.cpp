#include <pch.h>
#include "config.h"

namespace engine
{
	void to_json(nlohmann::json& j, const camera& p)
	{
		j = nlohmann::json{
			{"farPlane", p.farPlane},
			{"nearPlane", p.nearPlane},
			{"fov", p.fov},
		};
	}

	void from_json(const nlohmann::json& j, camera& p)
	{
		j.at("farPlane").get_to(p.farPlane);
		j.at("nearPlane").get_to(p.nearPlane);
		j.at("fov").get_to(p.fov);
	}

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

	void to_json(nlohmann::json& j, const editor& p)
	{
		j = nlohmann::json
		{
			{"name", p.name},
		};
	}

	void from_json(const nlohmann::json& j, editor& p)
	{
		j.at("name").get_to(p.name);
	}

	void to_json(nlohmann::json& j, const log& p)
	{
		j = nlohmann::json
		{
			{"file", p.file},
			{"useFile", p.useFile},
			{"pattern", p.pattern},
			{"level", p.level},
		};
	}

	void from_json(const nlohmann::json& j, log& p)
	{
		j.at("file").get_to(p.file);
		j.at("useFile").get_to(p.useFile);
		j.at("pattern").get_to(p.pattern);
		j.at("level").get_to(p.level);
	}

	void to_json(nlohmann::json& j, const wnd& p)
	{
		j = nlohmann::json{
			{"width", p.width},
			{"height", p.height},
			{"name", p.name},
			{"showCursor", p.showCursor},
		};
	}

	void from_json(const nlohmann::json& j, wnd& p)
	{
		j.at("width").get_to(p.width);
		j.at("height").get_to(p.height);
		j.at("name").get_to(p.name);
		j.at("showCursor").get_to(p.showCursor);
	}

	void to_json(nlohmann::json& j, const main& p)
	{
		j = nlohmann::json
		{
			{"application", p.app},
			{"logger", p.log},
			{"wnd", p.wnd},
			{"gameLoop", p.gameLoop},
			{"camera", p.camera},
			{"render", p.render},
		};
	}

	void from_json(const nlohmann::json& j, main& p)
	{
		j.at("application").get_to(p.app);
		j.at("logger").get_to(p.log);
		j.at("wnd").get_to(p.wnd);
		j.at("gameLoop").get_to(p.gameLoop);
		j.at("camera").get_to(p.camera);
		j.at("render").get_to(p.render);
	}

	void from_json(const nlohmann::json& j, render& p)
	{
		j.at("shaderWorkGroup").get_to(p.shaderWorkGroup);
	}

	void to_json(nlohmann::json& j, const render& p)
	{
		j = nlohmann::json{
			{"shaderWorkGroup", p.shaderWorkGroup},
		};
	}
}