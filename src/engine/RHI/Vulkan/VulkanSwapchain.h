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
        VkSwapchainKHR GetSwapchain() const { return m_Swapchain; }
        const std::vector<VkImage>& GetImages() const { return m_Images; }
        VkExtent2D GetExtent() const { return m_Extent; }
        VkFormat GetImageFormat() const { return m_ImageFormat; }

    private:
        bool CreateSwapchain(VulkanContext& context, EZEngine::Platform::GlfwWindow& window);
        bool GetSwapchainImages();
        bool CreateImageViews();

    private:
        VkSwapchainKHR m_Swapchain = VK_NULL_HANDLE;
        std::vector<VkImage> m_Images;
        std::vector<VkImageView> m_ImageViews;

        VkFormat m_ImageFormat = VK_FORMAT_UNDEFINED;
        VkExtent2D m_Extent{};

        VkDevice m_Device = VK_NULL_HANDLE;
    };
}