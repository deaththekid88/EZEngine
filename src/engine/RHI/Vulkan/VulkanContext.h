#pragma once
#include "engine/Platform/GlfwWindow.h"
#include <vulkan/vulkan.h>
#include <optional>

namespace EZEngine::RHI
{
    struct QueueFamilyIndices
    {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;

        bool IsComplete() const
        {
            return graphicsFamily.has_value() && presentFamily.has_value();
        }
    };

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

        bool PickPhysicalDevice();
        bool CreateLogicalDevice();

        QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device);
    
    private:
        VkInstance m_instance = VK_NULL_HANDLE;
        VkDebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;
        VkSurfaceKHR m_surface = VK_NULL_HANDLE;
        VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE; 
        VkDevice m_device = VK_NULL_HANDLE;

        VkQueue m_graphicsQueue = VK_NULL_HANDLE;
        VkQueue m_presentQueue = VK_NULL_HANDLE;
    };
}