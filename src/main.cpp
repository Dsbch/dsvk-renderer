#include "pch.h"
#include <glad/glad.h>
#include "engine/events/dispatcher.h"
#include "engine/events/events.h"
#include "engine/window/windowFactory.h"
#include "../core/console/console.h"
#include "engine/renderer/rendererFactory.h"

void resize(const engine::windowResizeEvent& e)
{
    LOGINFO("New width {}, new height {}", e.width(), e.height());
}

int WINAPI WinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPSTR     lpCmdLine,
    _In_ int       nCmdShow
)
{
#ifdef DEBUG
    auto c = core::createConsole();
#endif // DEBUG

    engine::windowResizeEvent e{12, 12};
    engine::eventDispatcher::addHandler<engine::windowResizeEvent>(e, resize);

    auto f = engine::windowFactory::createWindow("test", 1920, 1080, true, "flex", nCmdShow);
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

    //LOGINFO("OpenGL version: {}", reinterpret_cast<const char*>(glGetString(GL_VERSION)));

    f->updateWindowState();

#ifdef DEBUG
    DUMP_PROFILING("prof.json");
#endif // !DEBUG

    return 0;
}
