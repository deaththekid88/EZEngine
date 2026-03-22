#include "engine/RHI/Vulkan/VulkanSwapchain.h"
#include "engine/RHI/Vulkan/VulkanContext.h"
#include "engine/Platform/GlfwWindow.h"
#include "engine/Core/Log.h"

#include <vector>

using namespace EZEngine::Core;
using namespace EZEngine::Platform;

namespace EZEngine::RHI
{
    bool VulkanSwapchain::Initialize(VulkanContext& context, GlfwWindow& window)
    {
        m_Device = context.GetDevice();

        if (!CreateSwapchain(context, window))
            return false;

        if (!GetSwapchainImages())
            return false;

        Log("Swapchain created successfully.", LogType::INFO);
        return true;
    }

    void VulkanSwapchain::Shutdown()
    {
        if (m_Swapchain != VK_NULL_HANDLE)
        {
            vkDestroySwapchainKHR(m_Device, m_Swapchain, nullptr);
            m_Swapchain = VK_NULL_HANDLE;
        }
    }

    bool VulkanSwapchain::GetSwapchainImages()
    {
        uint32_t imageCount = 0;
        vkGetSwapchainImagesKHR(m_Device, m_Swapchain, &imageCount, nullptr);

        m_Images.resize(imageCount);
        vkGetSwapchainImagesKHR(m_Device, m_Swapchain, &imageCount, m_Images.data());

        Log("Swapchain image count: " + std::to_string(imageCount), LogType::INFO);
        return true;
    }

    bool VulkanSwapchain::CreateSwapchain(VulkanContext& context, GlfwWindow& window)
    {
        VkPhysicalDevice physicalDevice = context.GetPhysicalDevice();
        VkSurfaceKHR surface = context.GetSurface();
        QueueFamilyIndices indices = context.GetQueueFamilyIndices();

        VkSurfaceCapabilitiesKHR capabilities{};
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &capabilities);

        uint32_t formatCount = 0;
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);

        std::vector<VkSurfaceFormatKHR> formats(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, formats.data());

        uint32_t presentModeCount = 0;
        vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr);

        std::vector<VkPresentModeKHR> presentModes(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, presentModes.data());

        VkSurfaceFormatKHR chosenFormat = formats[0];
        for (const auto& format : formats)
        {
            if (format.format == VK_FORMAT_B8G8R8A8_SRGB &&
                format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            {
                chosenFormat = format;
                break;
            }
        }

        VkPresentModeKHR chosenPresentMode = VK_PRESENT_MODE_FIFO_KHR;
        for (const auto& mode : presentModes)
        {
            if (mode == VK_PRESENT_MODE_MAILBOX_KHR)
            {
                chosenPresentMode = mode;
                break;
            }
        }

        VkExtent2D extent = capabilities.currentExtent;

        uint32_t imageCount = capabilities.minImageCount + 1;
        if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount)
        {
            imageCount = capabilities.maxImageCount;
        }

        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = surface;
        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = chosenFormat.format;
        createInfo.imageColorSpace = chosenFormat.colorSpace;
        createInfo.imageExtent = extent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        uint32_t queueFamilyIndices[] = {
            indices.graphicsFamily.value(),
            indices.presentFamily.value()
        };

        if (indices.graphicsFamily != indices.presentFamily)
        {
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = queueFamilyIndices;
        }
        else
        {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            createInfo.queueFamilyIndexCount = 0;
            createInfo.pQueueFamilyIndices = nullptr;
        }

        createInfo.preTransform = capabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode = chosenPresentMode;
        createInfo.clipped = VK_TRUE;
        createInfo.oldSwapchain = VK_NULL_HANDLE;

        if (vkCreateSwapchainKHR(m_Device, &createInfo, nullptr, &m_Swapchain) != VK_SUCCESS)
        {
            Log("Failed to create swapchain.", LogType::ERROR);
            return false;
        }

        m_ImageFormat = chosenFormat.format;
        m_Extent = extent;

        Log("Swapchain extent: " + std::to_string(m_Extent.width) + "x" + std::to_string(m_Extent.height), LogType::INFO);
        Log("Swapchain format selected.", LogType::INFO);

        return true;
    }
}