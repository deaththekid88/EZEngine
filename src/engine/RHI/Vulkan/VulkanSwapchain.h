#pragma once
#include <engine/RHI/Vulkan/VulkanContext.h>
#include <vector>

namespace EZEngine::Platform
{
    class GlfwWindow;
}

namespace EZEngine::RHI
{
    struct SwapChainSupportDetails
    {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };

    class VulkanContext;

    class VulkanSwapchain
    {
    public:
        bool Initialize(VulkanContext &context, EZEngine::Platform::GlfwWindow &window);
        void Shutdown();
        VkSwapchainKHR GetSwapchain() const { return m_Swapchain; }
        const std::vector<VkImage> &GetImages() const { return m_Images; }
        VkExtent2D GetExtent() const { return m_Extent; }
        VkFormat GetImageFormat() const { return m_ImageFormat; }
        std::vector<VkImageView> GetImageViews() const { return m_ImageViews; }
        int GetImageCount() const { return static_cast<int>(m_Images.size()); }
        bool RecreateSwapchain(VulkanContext &context, EZEngine::Platform::GlfwWindow &window);

    private:
        bool CreateSwapchain(VulkanContext &context, EZEngine::Platform::GlfwWindow &window);
        bool GetSwapchainImages();
        bool CreateImageViews();
        bool Cleanup();

    private:
        VkSwapchainKHR m_Swapchain = VK_NULL_HANDLE;
        std::vector<VkImage> m_Images;
        std::vector<VkImageView> m_ImageViews;

        VkFormat m_ImageFormat = VK_FORMAT_UNDEFINED;
        VkExtent2D m_Extent{};

        VkDevice m_Device = VK_NULL_HANDLE;
    };
}