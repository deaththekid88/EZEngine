#include "GlfwWindow.h"
#include "engine/Core/Log.h"
#include "engine/Core/Events.h"

using namespace EZEngine::Core;

namespace EZEngine::Platform
{
    static void GlfwErrorCallback(int code, const char* desc)
    {
        EZEngine::Core::Log("GLFW Error " + std::to_string(code) + ": " + (desc ? desc : "(null)"), EZEngine::Core::LogType::ERROR);
    }

    static void FramebufferSizeCallback(GLFWwindow* window, int width, int height)
    {
        auto* glfwWindow = static_cast<GlfwWindow*>(glfwGetWindowUserPointer(window));
        if (!glfwWindow)
            return;
        
        glfwWindow->PushResizeEvent(width, height);
    }

    static void WindowCloseCallback(GLFWwindow* window)
    {
        auto* glfwWindow = static_cast<GlfwWindow*>(glfwGetWindowUserPointer(window));
        if (!glfwWindow)
            return;
        
        glfwWindow->PushCloseEvent();
    }

    bool GlfwWindow::Create(const WindowDescription& desc, EZEngine::Core::EventQueue* eventQueue)
    {
        m_EventQueue = eventQueue;
        glfwSetErrorCallback(GlfwErrorCallback);
        if (!glfwInit())
        {
            Log("Failed to initialize GLFW", LogType::ERROR);
            return false;
        }
       glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

        m_Window = glfwCreateWindow(desc.Width, desc.Height, desc.Title.c_str(), nullptr, nullptr);
        if (!m_Window)
        {
            Log("glfwCreateWindow failed.", LogType::ERROR);
            glfwTerminate();
            return false;
        }

        glfwSetWindowUserPointer(m_Window, this);
        glfwSetFramebufferSizeCallback(m_Window, FramebufferSizeCallback);
        glfwSetWindowCloseCallback(m_Window, WindowCloseCallback);
        Log("Window created.", LogType::INFO);
        return true;
    }

    GlfwWindow::~GlfwWindow()
    {
        if (m_Window)
        {
            glfwDestroyWindow(m_Window);
            m_Window = nullptr;
            Log("GLFW window destroyed", LogType::INFO);
        }
        glfwTerminate();
    }

    void GlfwWindow::PollEvents()
    {
        glfwPollEvents();
    }

    bool GlfwWindow::ShouldClose() const
    {
        return m_Window && glfwWindowShouldClose(m_Window);
    }

    void GlfwWindow::RequestClose()
    {
        if (m_Window)
        {
            glfwSetWindowShouldClose(m_Window, true);
        }
    }

    bool GlfwWindow::IsKeyDown(int key) const
    {
        if(!m_Window)
            return false;
        return glfwGetKey(m_Window, key) == GLFW_PRESS;
    }

    void GlfwWindow::PushResizeEvent(int width, int height)
    {
        if(!m_EventQueue)
            return;
        m_EventQueue->Push(WindowResizeEvent{ width, height });
    }

    void GlfwWindow::PushCloseEvent()
    {
        if(!m_EventQueue)
            return;
        m_EventQueue->Push(WindowCloseEvent{});
    }
}