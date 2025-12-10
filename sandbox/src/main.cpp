#include <application/application.h>
#include <core/scene/systems/system.h>

class sandboxSystem : public engine::system
{
public:
	sandboxSystem(std::shared_ptr<engine::context> ctx) : engine::system(ctx) {};
	
	engine::error checkError()
	{
		return {};
	}

	engine::error onUpdate(entt::registry& registry)
	{
		return {};
	}

	engine::error onRender(entt::registry& registry)
	{
		return {};
	}

	engine::error onEvent(entt::registry& registry, std::shared_ptr<engine::baseEvent> e)
	{
		return {};
	}
};

int main(int argc, char* argv[])
{
	try
	{
		engine::application app;
		engine::error err = app.checkError();
		if (err)
		{
			LOGERROR(err.err());
			return 0;
		}

		auto ss = std::make_unique<sandboxSystem>(app.getAppContext());

		app.addUserSystem(std::move(ss));
		err = app.checkError();
		if (err)
		{
			LOGERROR(err.err());
			return 0;
		}

		err = app.run();
		if (err)
		{
			LOGERROR(err.err());
			return 0;
		}
	}
	catch (...)
	{
		LOGERROR("exception was caught in run");
		return 0;
	}

	return 0;
}
