#pragma once
#include <chrono>

namespace EZEngine::Core
{
    class Time
    {
    public:
        using clock = std::chrono::steady_clock;

        static double NowSeconds()
        {
            return std::chrono::duration<double>(clock::now().time_since_epoch()).count();
        }
    };
}