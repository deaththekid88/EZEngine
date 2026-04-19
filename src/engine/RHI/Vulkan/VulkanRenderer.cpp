#include "engine/RHI/Vulkan/VulkanRenderer.h"
#include "engine/RHI/Vulkan/VulkanContext.h"
#include "engine/Platform/GlfwWindow.h"
#include "engine/Core/Log.h"

using namespace EZEngine::Core;

namespace EZEngine::RHI
{
    VulkanRenderer::VulkanRenderer(VulkanContext &context, VulkanSwapchain &swapchain)
        : m_Context(context), m_Swapchain(swapchain)
    {
    }

    VulkanRenderer::~VulkanRenderer()
    {
    }
    bool VulkanRenderer::Initialize()
    {
        m_Device = m_Context.GetDevice();
        MAX_FRAMES_IN_FLIGHT = static_cast<uint32_t>(m_Swapchain.GetImages().size());

        if (!CreateCommandPool())
            return false;

        if (!AllocateCommandBuffer())
            return false;

        if (!CreateSyncObjects())
            return false;

        if (!CreateRenderPass())
            return false;

        Log("Vulkan renderer initialized.", LogType::INFO);
        return true;
    }

    void VulkanRenderer::Shutdown()
    {
        for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            if (m_ImageAvailableSemaphores[i] != VK_NULL_HANDLE)
                vkDestroySemaphore(m_Device, m_ImageAvailableSemaphores[i], nullptr);
            if (m_RenderFinishedSemaphores[i] != VK_NULL_HANDLE)
                vkDestroySemaphore(m_Device, m_RenderFinishedSemaphores[i], nullptr);
            if (m_InFlightFences[i] != VK_NULL_HANDLE)
                vkDestroyFence(m_Device, m_InFlightFences[i], nullptr);
        }

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
        m_CommandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = m_CommandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = static_cast<uint32_t>(m_CommandBuffers.size());

        if (vkAllocateCommandBuffers(m_Device, &allocInfo, m_CommandBuffers.data()) != VK_SUCCESS)
        {
            Log("Failed to allocate command buffer.", LogType::ERROR);
            return false;
        }

        Log("Command buffer allocated.", LogType::INFO);
        return true;
    }

    void VulkanRenderer::RenderFrame(EZEngine::Platform::GlfwWindow &window)
    {
        if (window.WasResized())
        {
            window.ResetResized();
            RecreateSwapchain(window);
            return;
        }

        VkQueue graphicsQueue = m_Context.GetGraphicsQueue();
        VkQueue presentQueue = m_Context.GetPresentQueue();
        VkSwapchainKHR swapchain = m_Swapchain.GetSwapchain();

        VkFence inFlightFence = m_InFlightFences[m_CurrentFrame];
        VkSemaphore ImageAvailableSemaphore = m_ImageAvailableSemaphores[m_CurrentFrame];
        VkSemaphore RenderFinishedSemaphore = m_RenderFinishedSemaphores[m_CurrentFrame];
        VkCommandBuffer commandBuffer = m_CommandBuffers[m_CurrentFrame];

        vkWaitForFences(m_Device, 1, &inFlightFence, VK_TRUE, UINT64_MAX);
        vkResetFences(m_Device, 1, &inFlightFence);

        uint32_t imageIndex = 0;

        VkResult result = vkAcquireNextImageKHR(
            m_Device,
            swapchain,
            UINT64_MAX,
            ImageAvailableSemaphore,
            VK_NULL_HANDLE,
            &imageIndex);

        if (result == VK_ERROR_OUT_OF_DATE_KHR)
        {
            RecreateSwapchain(window);
            return;
        }
        if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
        {
            throw std::runtime_error("Failed to acquire next swapchain image!");
        }

        RecordCommandBuffer(commandBuffer, imageIndex);

        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_TRANSFER_BIT};

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = &ImageAvailableSemaphore;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = &RenderFinishedSemaphore;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        result = vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFence);
        if (result != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to submit draw command buffer!");
        }

        VkSwapchainKHR swapchains[] = {swapchain};

        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &RenderFinishedSemaphore;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapchains;
        presentInfo.pImageIndices = &imageIndex;

        result = vkQueuePresentKHR(presentQueue, &presentInfo);
        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
        {
            RecreateSwapchain(window);
            return;
        }
        if (result != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to present swapchain image!");
        }

        m_CurrentFrame = (m_CurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }

    void VulkanRenderer::RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex)
    {
        VkExtent2D extent = m_Swapchain.GetExtent();
        VkImage image = m_Swapchain.GetImages()[imageIndex];

        vkResetCommandBuffer(commandBuffer, 0);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        VkResult result = vkBeginCommandBuffer(commandBuffer, &beginInfo);
        if (result != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to begin command buffer!");
        }

        TransitionImageLayout(
            commandBuffer,
            image,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        VkClearColorValue clearColor = {{0.1f, 0.1f, 0.3f, 1.0f}};

        VkImageSubresourceRange range{};
        range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        range.baseMipLevel = 0;
        range.levelCount = 1;
        range.baseArrayLayer = 0;
        range.layerCount = 1;

        vkCmdClearColorImage(
            commandBuffer,
            image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            &clearColor,
            1,
            &range);

        TransitionImageLayout(
            commandBuffer,
            image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

        result = vkEndCommandBuffer(commandBuffer);
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

    void VulkanRenderer::RecreateSwapchain(EZEngine::Platform::GlfwWindow &window)
    {
        vkDeviceWaitIdle(m_Device);

        if (!m_Swapchain.RecreateSwapchain(m_Context, window))
        {
            throw std::runtime_error("Failed to recreate swapchain!");
        }

        Log("Swapchain recreated.", LogType::INFO);
    }

    bool VulkanRenderer::CreateSyncObjects()
    {
        m_ImageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        m_RenderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        m_InFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            if (vkCreateSemaphore(m_Device, &semaphoreInfo, nullptr, &m_ImageAvailableSemaphores[i]) != VK_SUCCESS ||
                vkCreateSemaphore(m_Device, &semaphoreInfo, nullptr, &m_RenderFinishedSemaphores[i]) != VK_SUCCESS ||
                vkCreateFence(m_Device, &fenceInfo, nullptr, &m_InFlightFences[i]) != VK_SUCCESS)
            {
                Log("Failed to create sync objects.", LogType::ERROR);
                return false;
            }
        }

        Log("Sync objects created.", LogType::INFO);
        return true;
    }

    bool VulkanRenderer::CreateRenderPass()
    {
        VkFormat swapchainFormat = m_Swapchain.GetImageFormat();

        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = swapchainFormat;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

        VkAttachmentReference colorAttachmentRef{};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;

        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = 1;
        renderPassInfo.pAttachments = &colorAttachment;
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;

        if (vkCreateRenderPass(m_Device, &renderPassInfo, nullptr, &m_RenderPass) != VK_SUCCESS)
        {
            Log("Failed to create render pass.", LogType::ERROR);
            return false;
        }

        Log("Render pass created.", LogType::INFO);
        return true;
    }
}