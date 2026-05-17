#define GLFW_INCLUDE_VULKAN
#include "engine/RHI/Vulkan/VulkanContext.h"
#include "engine/Platform/GlfwWindow.h"
#include "engine/Core/Log.h"
#include <GLFW/glfw3.h>
#include <vector>
#include <cstring>
#include <set>

using namespace EZEngine::Core;
using namespace EZEngine::Platform;

namespace EZEngine::RHI
{
#ifdef NDEBUG
    const bool enableValidationLayers = false;
#else
    const bool enableValidationLayers = true;
#endif

    static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
        void *pUserData)
    {
        (void)messageType;
        (void)pUserData;

        if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
            Log("[Vulkan] " + std::string(pCallbackData->pMessage), LogType::ERROR);
        else if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
            Log("[Vulkan] " + std::string(pCallbackData->pMessage), LogType::WARNING);
        else
            Log("[Vulkan] " + std::string(pCallbackData->pMessage), LogType::INFO);

        return VK_FALSE;
    }

    static VkResult CreateDebugUtilsMessengerEXT(
        VkInstance instance,
        const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
        const VkAllocationCallbacks *pAllocator,
        VkDebugUtilsMessengerEXT *pDebugMessenger)
    {
        auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
        if (func)
            return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
        else
            return VK_ERROR_EXTENSION_NOT_PRESENT;
    }

    static void DestroyDebugUtilsMessengerEXT(
        VkInstance instance,
        VkDebugUtilsMessengerEXT debugMessenger,
        const VkAllocationCallbacks *pAllocator)
    {
        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
        if (func)
            func(instance, debugMessenger, pAllocator);
    }

    bool VulkanContext::Initialize(GlfwWindow &window)
    {
        if (!CreateInstance())
            return false;
        if (!SetupDebugMessenger())
            return false;
        // Surface는 물리 디바이스 선택 전에 만들어야 함 (physical device가 surface 지원하는지 체크해야 하므로)
        if (!CreateSurface(window))
            return false;
        if (!EnumeratePhysicalDevices())
            return false;
        if (!PickPhysicalDevice())
            return false;
        if (!CreateLogicalDevice())
            return false;

        Log("Vulkan context initialized successfully.", LogType::INFO);
        return true;
    }

    void VulkanContext::Shutdown()
    {
        if (m_logiclalDevice != VK_NULL_HANDLE)
        {
            vkDestroyDevice(m_logiclalDevice, nullptr);
            m_logiclalDevice = VK_NULL_HANDLE;
        }

        if (m_surface != VK_NULL_HANDLE)
        {
            vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
            m_surface = VK_NULL_HANDLE;
        }

        if (m_debugMessenger != VK_NULL_HANDLE)
        {
            DestroyDebugUtilsMessengerEXT(m_instance, m_debugMessenger, nullptr);
            m_debugMessenger = VK_NULL_HANDLE;
        }

        if (m_instance != VK_NULL_HANDLE)
        {
            vkDestroyInstance(m_instance, nullptr);
            m_instance = VK_NULL_HANDLE;
        }

        Log("Vulkan context shut down.", LogType::INFO);
    }

    bool VulkanContext::CreateInstance()
    {
        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "EZEngine";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "EZEngine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_0;

        uint32_t glfwExtensionCount = 0;
        const char **glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        std::vector<const char *> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

        if (enableValidationLayers)
        {
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }

        const std::vector<const char *> validationLayers = {
            "VK_LAYER_KHRONOS_validation"};

        if (enableValidationLayers && !checkValidationLayerSupport(validationLayers))
        {
            Log("Validation layers requested, but not available!", LogType::ERROR);
            return false;
        }

        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;
        createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        createInfo.ppEnabledExtensionNames = extensions.data();
        // 유효성 검사 레이어는 디버그 모드에서만 활성화
        if (enableValidationLayers)
        {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();
        }
        else
        {
            createInfo.enabledLayerCount = 0;
            createInfo.ppEnabledLayerNames = nullptr;
        }

        VkResult result = vkCreateInstance(&createInfo, nullptr, &m_instance);
        if (result != VK_SUCCESS)
        {
            Log("Failed to create Vulkan instance: " + std::to_string(result), LogType::ERROR);
            return false;
        }

        Log("Vulkan instance created.", LogType::INFO);
        return true;
    }

    bool checkValidationLayerSupport(const std::vector<const char *> &validationLayers)
    {
        uint32_t layerCount = 0;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        for (const char *layerName : validationLayers)
        {
            bool layerFound = false;
            for (const auto &layerProperties : availableLayers)
            {
                if (strcmp(layerName, layerProperties.layerName) == 0)
                {
                    layerFound = true;
                    break;
                }
            }

            if (false == layerFound)
            {
                return false;
            }
        }

        return true;
    }

    bool VulkanContext::SetupDebugMessenger()
    {
        if (!enableValidationLayers)
            return true;

        VkDebugUtilsMessengerCreateInfoEXT createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                     VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                     VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = DebugCallback;

        if (CreateDebugUtilsMessengerEXT(m_instance, &createInfo, nullptr, &m_debugMessenger) != VK_SUCCESS)
        {
            Log("Failed to set up debug messenger.", LogType::ERROR);
            return false;
        }

        Log("Debug messenger set up.", LogType::INFO);
        return true;
    }

    bool VulkanContext::CreateSurface(GlfwWindow &window)
    {
        VkSurfaceKHR surface = VK_NULL_HANDLE;

        GLFWwindow *glfwWindow = window.GetGlfwWindow();
        VkResult result = glfwCreateWindowSurface(m_instance, glfwWindow, nullptr, &surface);
        if (result != VK_SUCCESS)
        {
            Log("Failed to create window surface.", LogType::ERROR);
            return false;
        }

        m_surface = surface;
        Log("Window surface created.", LogType::INFO);
        return true;
    }

    bool VulkanContext::EnumeratePhysicalDevices()
    {
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);
        if (deviceCount == 0)
        {
            Log("Failed to find GPUs with Vulkan support.", LogType::ERROR);
            return false;
        }

        m_physicalDevices.resize(deviceCount);
        vkEnumeratePhysicalDevices(m_instance, &deviceCount, m_physicalDevices.data());

        Log("Found " + std::to_string(deviceCount) + " physical device(s) with Vulkan support.", LogType::INFO);

        for (auto device : m_physicalDevices)
        {
            VkPhysicalDeviceProperties deviceProperties;
            vkGetPhysicalDeviceProperties(device, &deviceProperties);
            Log(" - " + std::string(deviceProperties.deviceName), LogType::INFO);
        }
        return true;
    }

    bool VulkanContext::PickPhysicalDevice()
    {
        for (const auto &device : m_physicalDevices)
        {
            QueueFamilyIndices indices = FindQueueFamilies(device);
            if (indices.IsComplete())
            {
                m_physicalDevice = device;
                m_queueFamilyIndices = indices;

                VkPhysicalDeviceProperties deviceProperties;
                vkGetPhysicalDeviceProperties(device, &deviceProperties);

                Log("Selected GPU: " + std::string(deviceProperties.deviceName), LogType::INFO);
                Log("Graphics Queue Family: " + std::to_string(indices.graphicsFamily.value()), LogType::INFO);
                Log("Present Queue Family: " + std::to_string(indices.presentFamily.value()), LogType::INFO);
                return true;
            }
        }

        Log("Failed to find a suitable GPU.", LogType::ERROR);
        return false;
    }

    QueueFamilyIndices VulkanContext::FindQueueFamilies(VkPhysicalDevice device)
    {
        QueueFamilyIndices indices;

        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

        int i = 0;
        for (const auto &queueFamily : queueFamilies)
        {
            if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
                indices.graphicsFamily = i;

            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_surface, &presentSupport);

            if (presentSupport)
                indices.presentFamily = i;

            if (indices.IsComplete())
                break;

            i++;
        }
        return indices;
    }

    bool VulkanContext::CreateLogicalDevice()
    {
        const QueueFamilyIndices &indices = m_queueFamilyIndices;

        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsFamily.value(), indices.presentFamily.value()};

        float queuePriority = 1.0f; // 명령 버퍼 스케줄링 우선순위 (0.0 ~ 1.0)
        for (uint32_t queueFamily : uniqueQueueFamilies)
        {
            VkDeviceQueueCreateInfo queueCreateInfo{};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = queueFamily;
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &queuePriority;
            queueCreateInfos.push_back(queueCreateInfo);
        }

        VkPhysicalDeviceFeatures deviceFeatures{};

        VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
        createInfo.pQueueCreateInfos = queueCreateInfos.data();
        createInfo.pEnabledFeatures = &deviceFeatures;

        const char *deviceExtensions[] = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME};

        createInfo.enabledExtensionCount = 1;
        createInfo.ppEnabledExtensionNames = deviceExtensions;

        const char *validationLayers[] = {
            "VK_LAYER_KHRONOS_validation"};

        createInfo.enabledLayerCount = 1;
        createInfo.ppEnabledLayerNames = validationLayers;

        if (vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_logiclalDevice) != VK_SUCCESS)
        {
            Log("Failed to create logical device.", LogType::ERROR);
            return false;
        }

        vkGetDeviceQueue(m_logiclalDevice, indices.graphicsFamily.value(), 0, &m_graphicsQueue);
        vkGetDeviceQueue(m_logiclalDevice, indices.presentFamily.value(), 0, &m_presentQueue);

        Log("Logical device created.", LogType::INFO);
        return true;
    }
}