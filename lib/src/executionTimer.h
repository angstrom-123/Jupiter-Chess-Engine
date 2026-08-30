#pragma once 

#include <cstdint>
#include <chrono>

namespace chrono = std::chrono;

class ExecutionTimer {
public:
    ExecutionTimer();
    uint64_t Now() const;
    uint64_t StartTime() const;
    uint64_t SinceStart() const;

private:
    chrono::time_point<chrono::system_clock, chrono::duration<long, std::ratio<1, 1000000000>>> m_StartPoint;
};
