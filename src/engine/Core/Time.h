#pragma once
#include <chrono>
using namespace std::chrono;

namespace EZEngine::Core
{
    class Time
    {
    public:
        using clock = steady_clock;

        static double NowSeconds()
        {
            return duration<double>(clock::now().time_since_epoch()).count();
        }
    };
}