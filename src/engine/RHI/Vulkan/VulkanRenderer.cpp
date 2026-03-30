#include "engine/RHI/Vulkan/VulkanRenderer.h"
#include "engine/RHI/Vulkan/VulkanContext.h"
#include "engine/Core/Log.h"

using namespace EZEngine::Core;

namespace EZEngine::RHI
{
    bool VulkanRenderer::Initialize(VulkanContext& context)
    {
        m_Device = context.GetDevice();

        if (!CreateCommandPool(context))
            return false;

        if (!AllocateCommandBuffer(context))
            return false;

        Log("Vulkan renderer initialized.", LogType::INFO);
        return true;
    }

    void VulkanRenderer::Shutdown()
    {
        if (m_CommandPool != VK_NULL_HANDLE)
        {
            vkDestroyCommandPool(m_Device, m_CommandPool, nullptr);
            m_CommandPool = VK_NULL_HANDLE;
        }
    }

    bool VulkanRenderer::CreateCommandPool(VulkanContext& context)
    {
        QueueFamilyIndices indices = context.GetQueueFamilyIndices();

        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolInfo.queueFamilyIndex = indices.graphicsFamily.value();

        if (vkCreateCommandPool(m_Device, &poolInfo, nullptr, &m_CommandPool) != VK_SUCCESS)
        {
            Log("Failed to create command pool.", LogType::ERROR);
            return false;
        }

        Log("Command pool created.", LogType::INFO);
        return true;
    }

    bool VulkanRenderer::AllocateCommandBuffer(VulkanContext& context)
    {
        (void)context;

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = m_CommandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = 1;

        if (vkAllocateCommandBuffers(m_Device, &allocInfo, &m_CommandBuffer) != VK_SUCCESS)
        {
            Log("Failed to allocate command buffer.", LogType::ERROR);
            return false;
        }

        Log("Command buffer allocated.", LogType::INFO);
        return true;
    }
}