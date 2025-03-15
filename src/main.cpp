#include "pch.h"
#include <glad/glad.h>
#include "engine/events/dispatcher.h"
#include "engine/events/events.h"
#include "engine/window/windowFactory.h"
#include "engine/renderer/rendererFactory.h"

void resize(const engine::windowResizeEvent& e)
{
    LOGINFO("New width {}, new height {}", e.width(), e.height());
}

int run()
{
    try
    {
        core::logger::initLogger("asd", "logs.log", "[%H:%M:%S] [%^%l%$] %v", core::logger::debug);

        engine::eventDispatcher::addHandler<engine::windowResizeEvent>(resize);
        engine::windowResizeEvent e{ 12, 12 };
        engine::eventDispatcher::dispatch(e);

        auto f = engine::windowFactory::createWindow("test", 1920, 1080, true, "flex");
        if (f->checkError())
        {
            LOGERROR(f->checkError().err());
            return -1;
        }

        auto err = f->makeOpenglContext();
        if (err)
        {
            LOGERROR(err.err());
            return -1;
        }

        auto renderer = engine::rendererFactory::createRenderer();
        if (auto err = renderer->check(); err)
        {
            LOGERROR(err.err());
            return -1;
        }

        LOGINFO(renderer->getVersion());

        f->updateWindowState();

#ifdef DEBUG
        DUMP_PROFILING("prof.json");
#endif // !DEBUG
    }
    catch (...)
    {
        LOGERROR("exception was caught in run");
    }

    return 0;
}

int WINAPI WinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPSTR     lpCmdLine,
    _In_ int       nCmdShow
)
{
    return run();
}
