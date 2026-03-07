#pragma once
#include "engine/Platform/GlfwWindow.h"
#include <vulkan/vulkan.h>

namespace EZEngine::RHI
{
    class VulkanContext
    {
    public:
        bool Initialize(EZEngine::Platform::GlfwWindow& window);
        void Shutdown();
    
    private:
        bool CreateInstance();
        bool SetupDebugMessenger();
        bool CreateSurface(EZEngine::Platform::GlfwWindow& window);
        bool EnumeratePhysicalDevices();

        VkInstance m_instance = VK_NULL_HANDLE;
        VkDebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;
        VkSurfaceKHR m_surface = VK_NULL_HANDLE;
    };
}