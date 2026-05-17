#include "engine/RHI/Vulkan/VulkanSwapchain.h"
#include "engine/RHI/Vulkan/VulkanContext.h"
#include "engine/Platform/GlfwWindow.h"
#include "engine/Core/Log.h"

#include <vector>
#include <set>

using namespace EZEngine::Core;
using namespace EZEngine::Platform;

namespace EZEngine::RHI
{
    const std::vector<const char *> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME};

    bool VulkanSwapchain::Initialize(VulkanContext &context, GlfwWindow &window)
    {
        m_Device = context.GetDevice();

        if (!CreateSwapchain(context, window))
            return false;

        if (!GetSwapchainImages())
            return false;

        if (!CreateImageViews())
            return false;

        Log("Swapchain created successfully.", LogType::INFO);
        return true;
    }

    void VulkanSwapchain::Shutdown()
    {
        for (auto imageView : m_ImageViews)
        {
            if (imageView != VK_NULL_HANDLE)
            {
                vkDestroyImageView(m_Device, imageView, nullptr);
            }
        }
        m_ImageViews.clear();

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

    bool CheckDeviceExtensionSupport(VkPhysicalDevice device)
    {
        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

        std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());
        for (const auto &extension : availableExtensions)
        {
            requiredExtensions.erase(extension.extensionName);
        }

        return requiredExtensions.empty();
    }

    SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface)
    {
        SwapChainSupportDetails swapChainSupport;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &swapChainSupport.capabilities);

        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
        if (formatCount != 0)
        {
            swapChainSupport.formats.resize(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, swapChainSupport.formats.data());
        }

        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);
        if (presentModeCount != 0)
        {
            swapChainSupport.presentModes.resize(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, swapChainSupport.presentModes.data());
        }

        return swapChainSupport;
    }

    VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats)
    {
        for (const auto &availableFormat : availableFormats)
        {
            if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
                availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            {
                return availableFormat;
            }
        }

        return availableFormats[0];
    }

    VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes)
    {
        for (const auto &availablePresentMode : availablePresentModes)
        {
            if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
            {
                return availablePresentMode;
            }
        }

        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities, GlfwWindow &window)
    {
        if (capabilities.currentExtent.width != UINT32_MAX)
        {
            return capabilities.currentExtent;
        }
        else
        {
            int width, height;
            glfwGetFramebufferSize(window.GetGlfwWindow(), &width, &height);

            VkExtent2D actualExtent = {
                static_cast<uint32_t>(width),
                static_cast<uint32_t>(height)};

            actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
            actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));

            return actualExtent;
        }
    }

    bool VulkanSwapchain::CreateSwapchain(VulkanContext &context, GlfwWindow &window)
    {
        VkPhysicalDevice physicalDevice = context.GetPhysicalDevice();
        VkSurfaceKHR surface = context.GetSurface();
        QueueFamilyIndices indices = context.GetQueueFamilyIndices();

        bool extensionsSupported = CheckDeviceExtensionSupport(physicalDevice);
        if (!extensionsSupported)
        {
            Log("Device does not support required swapchain extensions.", LogType::ERROR);
            return false;
        }

        SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(physicalDevice, surface);
        if (swapChainSupport.formats.empty() || swapChainSupport.presentModes.empty())
        {
            Log("Device does not support required swapchain formats or present modes.", LogType::ERROR);
            return false;
        }

        VkSurfaceFormatKHR chosenFormat = ChooseSwapSurfaceFormat(swapChainSupport.formats);
        VkPresentModeKHR chosenPresentMode = ChooseSwapPresentMode(swapChainSupport.presentModes);
        VkExtent2D extent = ChooseSwapExtent(swapChainSupport.capabilities, window);

        uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
        if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
        {
            imageCount = swapChainSupport.capabilities.maxImageCount;
        }

        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = surface;
        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = chosenFormat.format;
        createInfo.imageColorSpace = chosenFormat.colorSpace;
        createInfo.imageExtent = extent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

        uint32_t queueFamilyIndices[] = {
            indices.graphicsFamily.value(),
            indices.presentFamily.value()};

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

        createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
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

    bool VulkanSwapchain::CreateImageViews()
    {
        m_ImageViews.resize(m_Images.size());

        for (size_t i = 0; i < m_Images.size(); i++)
        {
            VkImageViewCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            createInfo.image = m_Images[i];
            createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            createInfo.format = m_ImageFormat;

            createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

            createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            createInfo.subresourceRange.baseMipLevel = 0;
            createInfo.subresourceRange.levelCount = 1;
            createInfo.subresourceRange.baseArrayLayer = 0;
            createInfo.subresourceRange.layerCount = 1;

            if (vkCreateImageView(m_Device, &createInfo, nullptr, &m_ImageViews[i]) != VK_SUCCESS)
            {
                Log("Failed to create image view for swapchain image " + std::to_string(i), LogType::ERROR);
                return false;
            }
        }

        Log("Swapchain image views created: " + std::to_string(m_ImageViews.size()), LogType::INFO);
        return true;
    }

    bool VulkanSwapchain::Cleanup()
    {
        for (auto imageView : m_ImageViews)
        {
            if (imageView != VK_NULL_HANDLE)
            {
                vkDestroyImageView(m_Device, imageView, nullptr);
            }
        }
        m_ImageViews.clear();

        if (m_Swapchain != VK_NULL_HANDLE)
        {
            vkDestroySwapchainKHR(m_Device, m_Swapchain, nullptr);
            m_Swapchain = VK_NULL_HANDLE;
        }

        return true;
    }

    bool VulkanSwapchain::RecreateSwapchain(VulkanContext &context, GlfwWindow &window)
    {
        if (!Cleanup())
            return false;

        if (!CreateSwapchain(context, window))
            return false;

        if (!GetSwapchainImages())
            return false;

        if (!CreateImageViews())
            return false;

        Log("Swapchain recreated successfully.", LogType::INFO);
        return true;
    }
}