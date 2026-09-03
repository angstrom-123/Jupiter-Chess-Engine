#pragma once

namespace loglevel {
    const int INFO = 0;
    const int WARN = 1;
    const int ERROR = 2;
};

#ifdef SILENT
    #define _LOG(level, message)
#else 
    #include <iostream>
    #if defined(_WIN32) || defined(_WIN64)
        #define _LOG(level, message) std::cerr << message
    #else
        static const std::string colorCodes[3] = { "\x1B[0m", "\x1b[33m", "\x1b[31m" };
        static const std::string clearCode = "\x1b[0m";
        #define _LOG(level, message) std::cerr << colorCodes[level] << message << clearCode
    #endif
#endif

#define INFO(message) _LOG(loglevel::INFO, message << std::endl)
#define WARN(message) _LOG(loglevel::WARN, message << std::endl)
#define ERROR(message) _LOG(loglevel::ERROR, message << std::endl)
#define QUOTE(message) #message
