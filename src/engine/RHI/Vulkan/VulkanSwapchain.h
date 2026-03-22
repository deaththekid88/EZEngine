#pragma once
#include <engine/RHI/Vulkan/VulkanContext.h>
#include <vector>

namespace EZEngine::Platform
{
    class GlfwWindow;
}

namespace EZEngine::RHI
{
    class VulkanContext;

    class VulkanSwapchain
    {
    public:
        bool Initialize(VulkanContext& context, EZEngine::Platform::GlfwWindow& window);
        void Shutdown();

    private:
        bool CreateSwapchain(VulkanContext& context, EZEngine::Platform::GlfwWindow& window);
        bool GetSwapchainImages();

    private:
        VkSwapchainKHR m_Swapchain = VK_NULL_HANDLE;
        std::vector<VkImage> m_Images;

        VkFormat m_ImageFormat = VK_FORMAT_UNDEFINED;
        VkExtent2D m_Extent{};

        VkDevice m_Device = VK_NULL_HANDLE;
    };
}