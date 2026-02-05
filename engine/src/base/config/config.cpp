#include <pch.h>
#include "config.h"

namespace engine
{
	void to_json(nlohmann::json& j, const cameraCfg& p)
	{
		j = nlohmann::json{
			{"farPlane", p.farPlane},
			{"nearPlane", p.nearPlane},
			{"fov", p.fov},
		};
	}

	void from_json(const nlohmann::json& j, cameraCfg& p)
	{
		j.at("farPlane").get_to(p.farPlane);
		j.at("nearPlane").get_to(p.nearPlane);
		j.at("fov").get_to(p.fov);
	}

	void to_json(nlohmann::json& j, const gameLoopCfg& p)
	{
		j = nlohmann::json{
			{"fps", p.fps},
			{"gups", p.gups},
			{"minimumFps", p.minimumFps},
		};
	}

	void from_json(const nlohmann::json& j, gameLoopCfg& p)
	{
		j.at("fps").get_to(p.fps);
		j.at("gups").get_to(p.gups);
		j.at("minimumFps").get_to(p.minimumFps);
	}

	void to_json(nlohmann::json& j, const editorCfg& p)
	{
		j = nlohmann::json
		{
			{"name", p.name},
		};
	}

	void from_json(const nlohmann::json& j, editorCfg& p)
	{
		j.at("name").get_to(p.name);
	}

	void to_json(nlohmann::json& j, const logCfg& p)
	{
		j = nlohmann::json
		{
			{"file", p.file},
			{"useFile", p.useFile},
			{"pattern", p.pattern},
			{"level", p.level},
		};
	}

	void from_json(const nlohmann::json& j, logCfg& p)
	{
		j.at("file").get_to(p.file);
		j.at("useFile").get_to(p.useFile);
		j.at("pattern").get_to(p.pattern);
		j.at("level").get_to(p.level);
	}

	void to_json(nlohmann::json& j, const wndCfg& p)
	{
		j = nlohmann::json{
			{"width", p.width},
			{"height", p.height},
			{"name", p.name},
			{"showCursor", p.showCursor},
		};
	}

	void from_json(const nlohmann::json& j, wndCfg& p)
	{
		j.at("width").get_to(p.width);
		j.at("height").get_to(p.height);
		j.at("name").get_to(p.name);
		j.at("showCursor").get_to(p.showCursor);
	}

	void to_json(nlohmann::json& j, const mainCfg& p)
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

	void from_json(const nlohmann::json& j, mainCfg& p)
	{
		j.at("application").get_to(p.app);
		j.at("logger").get_to(p.log);
		j.at("wnd").get_to(p.wnd);
		j.at("gameLoop").get_to(p.gameLoop);
		j.at("camera").get_to(p.camera);
		j.at("render").get_to(p.render);
	}

	void from_json(const nlohmann::json& j, renderCfg& p)
	{
		j.at("shaderWorkGroup").get_to(p.shaderWorkGroup);
	}

	void to_json(nlohmann::json& j, const renderCfg& p)
	{
		j = nlohmann::json{
			{"shaderWorkGroup", p.shaderWorkGroup},
		};
	}

	void from_json(const nlohmann::json& j, graphicsCfg& p)
	{
		j.at("msaa").get_to(p.msaa);
		j.at("anisotropicFiltering").get_to(p.anisotropicFiltering);
	}

	void to_json(nlohmann::json& j, const graphicsCfg& p)
	{
		j = nlohmann::json{
			{"msaa", p.msaa},
			{"anisotropicFiltering", p.anisotropicFiltering},
		};
	}

	void from_json(const nlohmann::json& j, meshletCfg& p)
	{
		j.at("maxVert").get_to(p.maxVert);
		j.at("maxTriangles").get_to(p.maxTriangles);
		j.at("coneWieght").get_to(p.coneWieght);
		j.at("errorLevel").get_to(p.errorLevel);
	}

	void to_json(nlohmann::json& j, const meshletCfg& p)
	{
		j = nlohmann::json{
			{"maxVert", p.maxVert},
			{"maxTriangles", p.maxTriangles},
			{"coneWieght", p.coneWieght},
			{"errorLevel", p.errorLevel},
		};
	}
}