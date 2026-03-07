#pragma once
#include <iostream>
#include <string>

namespace EZEngine::Core
{
    enum LogType
    {
        INFO,
        WARNING,
        ERROR
    };

    inline void Log(const std::string& message, LogType type = LogType::INFO)
    {
        switch (type)
        {
        case LogType::INFO:
            std::cout << "[INFO] " << message << "\n" << std::endl;
            break;
        case LogType::WARNING:
            std::cout << "[WARNING] " << message << "\n" << std::endl;
            break;
        case LogType::ERROR:
            std::cout << "[ERROR] " << message << "\n" << std::endl;
            break;
        }
    }
} 