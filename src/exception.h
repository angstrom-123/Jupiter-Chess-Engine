#pragma once

#include <stdexcept>
#include <string>
#include <sstream>

// TODO: Add stack trace (not dependent on glibc)
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
        ss << message << "\n\n==== C++ Stack Trace ====\n" << "TODO";
        return ss.str();
    }
};
