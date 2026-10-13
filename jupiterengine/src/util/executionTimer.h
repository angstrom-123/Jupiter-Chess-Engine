#pragma once 

#include <cstdint>
#include <chrono>

namespace chrono = std::chrono;

using TimePoint = chrono::time_point<chrono::steady_clock, chrono::nanoseconds>;

class ExecutionTimer {
public:
    ExecutionTimer();
    TimePoint Now() const;
    TimePoint StartTime() const;

private:
    TimePoint m_StartPoint;
};
