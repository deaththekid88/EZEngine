#pragma once
#include <vulkan/vulkan.h>
#include "engine/RHI/Vulkan/VulkanContext.h"

namespace EZEngine::RHI
{
    class VulkanRenderer
    {
    public:
        bool Initialize(VulkanContext& context);
        void Shutdown();

        VkCommandPool GetCommandPool() const { return m_CommandPool; }
        VkCommandBuffer GetCommandBuffer() const { return m_CommandBuffer; }
    
    private:
        bool CreateCommandPool(VulkanContext& context);
        bool AllocateCommandBuffer(VulkanContext& context);

    private:
        VkDevice m_Device = VK_NULL_HANDLE;
        VkCommandPool m_CommandPool = VK_NULL_HANDLE;
        VkCommandBuffer m_CommandBuffer = VK_NULL_HANDLE;
    };
}