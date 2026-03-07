#pragma once
#include <variant>
#include <vector>

namespace EZEngine::Core
{
    struct WindowResizeEvent
    {
        int width;
        int height;
    };

    struct WindowCloseEvent
    {
    };

    using Event = std::variant<WindowResizeEvent, WindowCloseEvent>;

    class EventQueue
    {
    public:
        void Push(const Event& event)
        {
            m_evnets.push_back(event);
        }

        std::vector<Event> Drain()
        {
            std::vector<Event> out;
            out.swap(m_evnets);
            return out;
        }
    private:
        std::vector<Event> m_evnets;
    };
}