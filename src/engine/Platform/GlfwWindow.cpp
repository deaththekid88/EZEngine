#include "GlfwWindow.h"
#include "engine/Core/Log.h"
using namespace EZEngine::Core;

namespace EZEngine::Platform
{
    static void GlfwErrorCallback(int code, const char* desc)
    {
        Log(string("GLFW Error ") + to_string(code) + ": " + (desc ? desc : "(null)"));
    }

    bool GlfwWindow::Create(const WindowDescription& desc)
    {
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
}