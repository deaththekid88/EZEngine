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

        VkInstance GetInstance() const { return m_instance; }
        VkPhysicalDevice GetPhysicalDevice() const { return m_physicalDevice; }
        VkDevice GetDevice() const { return m_logiclalDevice; }
        VkSurfaceKHR GetSurface() const { return m_surface; }
        QueueFamilyIndices GetQueueFamilyIndices() const { return m_queueFamilyIndices; }
    
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
        std::vector<VkPhysicalDevice> m_physicalDevices;
        VkDevice m_logiclalDevice = VK_NULL_HANDLE;

        VkQueue m_graphicsQueue = VK_NULL_HANDLE;
        VkQueue m_presentQueue = VK_NULL_HANDLE;

        QueueFamilyIndices m_queueFamilyIndices;
    };
}