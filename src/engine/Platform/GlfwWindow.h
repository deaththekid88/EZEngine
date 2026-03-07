#pragma once
#include <string>
#include <GLFW/glfw3.h>
#include "engine/Core/Events.h"

namespace EZEngine::Platform
{
    struct WindowDescription
    {
        int Width = 1280;
        int Height = 720;
        std::string Title = "EZEngine";
    };

    class GlfwWindow
    {
    public:
        GlfwWindow() = default;
        ~GlfwWindow();
        
        bool Create(const WindowDescription& desc, EZEngine::Core::EventQueue* eventQueue);
        void PollEvents();
        bool ShouldClose() const;
        void RequestClose();

        bool IsKeyDown(int key) const; // 조회용

        void PushResizeEvent(int width, int height);
        void PushCloseEvent();

        GLFWwindow* GetGlfwWindow() const { return m_Window; }

    private:
        GLFWwindow* m_Window = nullptr;
        EZEngine::Core::EventQueue* m_EventQueue = nullptr;
    };
}