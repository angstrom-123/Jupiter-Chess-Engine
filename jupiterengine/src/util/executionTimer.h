#pragma once 

#include <chrono>

namespace chrono = std::chrono;

using TimePoint = chrono::time_point<chrono::steady_clock, chrono::nanoseconds>;

class ExecutionTimer {
public:
    ExecutionTimer();
    TimePoint Now() const;
    TimePoint StartTime() const;
    template<typename T> T SinceStart()
    {
        return chrono::duration_cast<T>(Now() - m_StartPoint);
    }
    

private:
    TimePoint m_StartPoint;
};
