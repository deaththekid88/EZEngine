#define GLFW_INCLUDE_VULKAN
#include "engine/RHI/Vulkan/VulkanContext.h"
#include "engine/Platform/GlfwWindow.h"
#include "engine/Core/Log.h"
#include <GLFW/glfw3.h>
#include <vector>
#include <cstring>

using namespace EZEngine::Core;
using namespace EZEngine::Platform;

namespace EZEngine::RHI
{
    static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData)
    {
        (void)messageType;
        (void)pUserData;

        if(messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
            Log("[Vulkan] " + std::string(pCallbackData->pMessage), LogType::WARNING);
        else
            Log("[Vulkan] " + std::string(pCallbackData->pMessage), LogType::INFO);

        return VK_FALSE;
    }  

    static VkResult CreateDebugUtilsMessengerEXT(
        VkInstance instance, 
        const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, 
        const VkAllocationCallbacks* pAllocator, 
        VkDebugUtilsMessengerEXT* pDebugMessenger)
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
        const VkAllocationCallbacks* pAllocator)
    {
        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
        if (func)
            func(instance, debugMessenger, pAllocator);
    }
      
    bool VulkanContext::Initialize(GlfwWindow& window)
    {
        if (!CreateInstance())
            return false;
        if (!SetupDebugMessenger())
            return false;
        if (!CreateSurface(window))
            return false;
        if (!EnumeratePhysicalDevices())
            return false;

        Log("Vulkan context initialized successfully.", LogType::INFO);
        return true;
    }

    void VulkanContext::Shutdown()
    {
        if(m_surface != VK_NULL_HANDLE)
        {
            vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
            m_surface = VK_NULL_HANDLE;
        }

        if(m_debugMessenger != VK_NULL_HANDLE)
        {
            DestroyDebugUtilsMessengerEXT(m_instance, m_debugMessenger, nullptr);
            m_debugMessenger = VK_NULL_HANDLE;
        }

        if(m_instance != VK_NULL_HANDLE)
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
        const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

        const char* validationLayers[] = {
            "VK_LAYER_KHRONOS_validation"
        };

        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;
        createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        createInfo.ppEnabledExtensionNames = extensions.data();
        createInfo.enabledLayerCount = 1;
        createInfo.ppEnabledLayerNames = validationLayers;

        VkResult result = vkCreateInstance(&createInfo, nullptr, &m_instance);
        if (result != VK_SUCCESS)        {
            Log("Failed to create Vulkan instance: " + std::to_string(result), LogType::ERROR);
            return false;
        }

        Log("Vulkan instance created.", LogType::INFO);
        return true;
    }

    bool VulkanContext::SetupDebugMessenger()
    {
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

    bool VulkanContext::CreateSurface(GlfwWindow& window)
    {
        VkSurfaceKHR surface = VK_NULL_HANDLE;

        GLFWwindow* glfwWindow = window.GetGlfwWindow();
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

        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(m_instance, &deviceCount, devices.data());

        Log("Found " + std::to_string(deviceCount) + " physical device(s) with Vulkan support.", LogType::INFO);

        for (auto device : devices)
        {
            VkPhysicalDeviceProperties deviceProperties;
            vkGetPhysicalDeviceProperties(device, &deviceProperties);
            Log(" - " + std::string(deviceProperties.deviceName), LogType::INFO);
        }
        return true;
    }
}