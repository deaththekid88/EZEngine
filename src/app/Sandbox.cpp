#include "engine/Core/Log.h"
#include "engine/Core/Time.h"
#include "engine/Platform/GlfwWindow.h"

#include <string>

using namespace EZEngine::Core;
using namespace EZEngine::Platform;

int main()
{
    GlfwWindow window;
    if (!window.Create({}))
        return 1;

    Log("BOOT demo started: ESC to exit, FPS log every 1 second.", LogType::INFO);
    double last = Time::NowSeconds();
    double acc = 0.0;
    int frames = 0;

    while (!window.ShouldClose())
    {
        window.PollEvents();

        if (window.IsKeyDown(GLFW_KEY_ESCAPE))
            window.RequestClose();

        const double now = Time::NowSeconds();
        const double dt = now - last;
        last = now;

        acc += dt;
        frames++;

        if (acc >= 1.0)
        {
            const double fps = static_cast<double>(frames) / acc;
            Log("FPS: " + std::to_string(fps), LogType::INFO);
            acc = 0.0;
            frames = 0;
        }
    }

    Log("BOOT demo finished.", LogType::INFO);
    return 0;
}