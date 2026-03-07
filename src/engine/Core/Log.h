#pragma once
#include <iostream>
#include <string>
using namespace std;
namespace EZEngine::Core
{
    enum LogType
    {
        INFO,
        WARNING,
        ERROR
    };

    inline void Log(const string& message, LogType type = LogType::INFO)
    {
        switch (type)
        {
        case LogType::INFO:
            cout << "[INFO] " << message << "\n" << endl;
            break;
        case LogType::WARNING:
            cout << "[WARNING] " << message << "\n" << endl;
            break;
        case LogType::ERROR:
            cout << "[ERROR] " << message << "\n" << endl;
            break;
        }
    }
} 