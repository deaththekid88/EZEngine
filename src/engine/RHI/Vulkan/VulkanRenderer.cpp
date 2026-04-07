#include "engine/RHI/Vulkan/VulkanRenderer.h"
#include "engine/RHI/Vulkan/VulkanContext.h"
#include "engine/Core/Log.h"

using namespace EZEngine::Core;

namespace EZEngine::RHI
{
    VulkanRenderer::VulkanRenderer(VulkanContext& context, VulkanSwapchain& swapchain)
        : m_Context(context), m_Swapchain(swapchain)
    {
    }

    VulkanRenderer::~VulkanRenderer()
    {
    }
    bool VulkanRenderer::Initialize()
    {
        m_Device = m_Context.GetDevice();

        if (!CreateCommandPool())
            return false;

        if (!AllocateCommandBuffer())
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

    bool VulkanRenderer::CreateCommandPool()
    {
        QueueFamilyIndices indices = m_Context.GetQueueFamilyIndices();

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

    bool VulkanRenderer::AllocateCommandBuffer()
    {
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
    
    void VulkanRenderer::RenderFrame()
    {
        VkQueue graphicsQueue = m_Context.GetGraphicsQueue();
        VkQueue presentQueue = m_Context.GetPresentQueue();
        VkSwapchainKHR swapchain = m_Swapchain.GetSwapchain();

        uint32_t imageIndex = 0;

        VkResult result = vkAcquireNextImageKHR(
            m_Device,
            swapchain,
            UINT64_MAX,
            VK_NULL_HANDLE,
            VK_NULL_HANDLE,
            &imageIndex);
        
        if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
        {
            throw std::runtime_error("Failed to acquire next swapchain image!");
        }

        RecordCommandBuffer(imageIndex);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &m_CommandBuffer;

        result = vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
        if (result != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to submit draw command buffer!");
        }

        vkQueueWaitIdle(graphicsQueue);

        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &swapchain;
        presentInfo.pImageIndices = &imageIndex;

        result = vkQueuePresentKHR(presentQueue, &presentInfo);
        if (result != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to present swapchain image!");
        }

        vkQueueWaitIdle(presentQueue);
    }

    void VulkanRenderer::RecordCommandBuffer(uint32_t imageIndex)
    {
        VkExtent2D extent = m_Swapchain.GetExtent();
        VkImage image = m_Swapchain.GetImages()[imageIndex];

        vkResetCommandBuffer(m_CommandBuffer, 0);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        VkResult result = vkBeginCommandBuffer(m_CommandBuffer, &beginInfo);
        if (result != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to begin command buffer!");
        }

        TransitionImageLayout(
            m_CommandBuffer,
            image,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        VkClearColorValue clearColor = { { 0.1f, 0.1f, 0.3f, 1.0f } };

        VkImageSubresourceRange range{};
        range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        range.baseMipLevel = 0;
        range.levelCount = 1;
        range.baseArrayLayer = 0;
        range.layerCount = 1;

        vkCmdClearColorImage(
            m_CommandBuffer,
            image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            &clearColor,
            1,
            &range);

        TransitionImageLayout(
            m_CommandBuffer,
            image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

        result = vkEndCommandBuffer(m_CommandBuffer);
        if (result != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to end command buffer!");
        }
    }

    void VulkanRenderer::TransitionImageLayout(
        VkCommandBuffer commandBuffer,
        VkImage image,
        VkImageLayout oldLayout,
        VkImageLayout newLayout)
    {
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;

        VkPipelineStageFlags srcStage;
        VkPipelineStageFlags dstStage;

        if (oldLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR &&
            newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
        {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

            srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        }
        else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
                newLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
        {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = 0;

            srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            dstStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        }
        else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
            newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
        {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

            srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        }
        else
        {
            throw std::runtime_error("Unsupported layout transition!");
        }

        vkCmdPipelineBarrier(
            commandBuffer,
            srcStage,
            dstStage,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier);
    }
}