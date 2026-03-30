#include "engine/Core/Log.h"
#include "engine/Core/Time.h"
#include "engine/Core/Events.h"
#include "engine/Platform/GlfwWindow.h"
#include "engine/RHI/Vulkan/VulkanContext.h"
#include "engine/RHI/Vulkan/VulkanSwapchain.h"
#include "engine/RHI/Vulkan/VulkanRenderer.h"

#include <string>

using namespace EZEngine::Core;
using namespace EZEngine::Platform;

int main()
{
    EventQueue eventQueue;
    GlfwWindow window;
    if (!window.Create({}, &eventQueue))
        return 1;
    
    EZEngine::RHI::VulkanContext vulkanContext;
    if (!vulkanContext.Initialize(window))
        return 1;

    EZEngine::RHI::VulkanSwapchain swapchain;
    if (!swapchain.Initialize(vulkanContext, window))
        return 1;
    
    EZEngine::RHI::VulkanRenderer renderer;
    if (!renderer.Initialize(vulkanContext))
        return 1;

    Log("BOOT demo started: ESC to exit, FPS log every 1 second.", LogType::INFO);
    double last = Time::NowSeconds();
    double acc = 0.0;
    int frames = 0;

    while (!window.ShouldClose())
    {
        window.PollEvents();

        for(const auto& event : eventQueue.Drain())
        {
            std::visit([&](auto&& event)
            {
                using T = std::decay_t<decltype(event)>;
                if constexpr (std::is_same_v<T, WindowResizeEvent>)
                {
                    Log("Window resized: " + std::to_string(event.width) + "x" + std::to_string(event.height), LogType::INFO);
                }
                else if constexpr (std::is_same_v<T, WindowCloseEvent>)
                {
                    Log("Window close requested.", LogType::INFO);
                }
            }, event);
        }

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

    renderer.Shutdown();
    swapchain.Shutdown();
    vulkanContext.Shutdown();
    Log("BOOT demo finished.", LogType::INFO);
    return 0;
}