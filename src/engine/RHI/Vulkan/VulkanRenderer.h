#pragma once
#include <vulkan/vulkan.h>
#include "engine/RHI/Vulkan/VulkanContext.h"
#include "engine/RHI/Vulkan/VulkanSwapchain.h"

namespace EZEngine::Platform
{
    class GlfwWindow;
}

namespace EZEngine::RHI
{
    class VulkanRenderer
    {
    public:
        VulkanRenderer(VulkanContext &context, VulkanSwapchain &swapchain);
        ~VulkanRenderer();

        bool Initialize();
        void Shutdown();

        void RenderFrame(EZEngine::Platform::GlfwWindow &window);

        VkCommandPool GetCommandPool() const { return m_CommandPool; }
        const std::vector<VkCommandBuffer> &GetCommandBuffers() const { return m_CommandBuffers; }

    private:
        bool CreateCommandPool();
        bool AllocateCommandBuffer();
        bool CreateSyncObjects();
        bool CreateRenderPass();

        void RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
        void TransitionImageLayout(
            VkCommandBuffer commandBuffer,
            VkImage image,
            VkImageLayout oldLayout,
            VkImageLayout newLayout);
        void RecreateSwapchain(EZEngine::Platform::GlfwWindow &window);

    private:
        VulkanContext &m_Context;
        VulkanSwapchain &m_Swapchain;

        VkDevice m_Device = VK_NULL_HANDLE;
        VkCommandPool m_CommandPool = VK_NULL_HANDLE;
        VkRenderPass m_RenderPass = VK_NULL_HANDLE;

        std::vector<VkCommandBuffer> m_CommandBuffers;
        std::vector<VkSemaphore> m_ImageAvailableSemaphores;
        std::vector<VkSemaphore> m_RenderFinishedSemaphores;
        std::vector<VkFence> m_InFlightFences;

        uint32_t MAX_FRAMES_IN_FLIGHT = 0;
        uint32_t m_CurrentFrame = 0;
    };
}