#pragma once

#include "stackTrace.h"
#include <stdexcept>
#include <string>
#include <sstream>

class Exception : public std::runtime_error {
public:
    explicit Exception(const std::string& message)
        : std::runtime_error{Format(message)} {}

    explicit Exception(const char *message)
        : std::runtime_error{Format(message)} {}

private:
    static std::string Format(const std::string& message)
    {
        std::stringstream ss;
        ss << message << std::endl;
        StackTracer::PrintTrace();
        return ss.str();
    }
};
