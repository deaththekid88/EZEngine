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
        VulkanRenderer(VulkanContext& context, VulkanSwapchain& swapchain);
        ~VulkanRenderer();

        bool Initialize();
        void Shutdown();

        void RenderFrame(EZEngine::Platform::GlfwWindow& window);

        VkCommandPool GetCommandPool() const { return m_CommandPool; }
        VkCommandBuffer GetCommandBuffer() const { return m_CommandBuffer; }
    
    private:
        bool CreateCommandPool();
        bool AllocateCommandBuffer();

        void RecordCommandBuffer(uint32_t imageIndex);
        void TransitionImageLayout(
            VkCommandBuffer commandBuffer, 
            VkImage image, 
            VkImageLayout oldLayout, 
            VkImageLayout newLayout);
        void RecreateSwapchain(EZEngine::Platform::GlfwWindow& window);

    private:
        VulkanContext& m_Context;
        VulkanSwapchain& m_Swapchain;

        VkDevice m_Device = VK_NULL_HANDLE;
        VkCommandPool m_CommandPool = VK_NULL_HANDLE;
        VkCommandBuffer m_CommandBuffer = VK_NULL_HANDLE;
    };
}