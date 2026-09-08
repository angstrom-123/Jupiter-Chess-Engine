#include "executionTimer.h"
#include <chrono>

ExecutionTimer::ExecutionTimer()
{
    m_StartPoint = chrono::high_resolution_clock::now();
}

uint64_t ExecutionTimer::Now() const
{
    return chrono::time_point_cast<chrono::milliseconds>(std::chrono::high_resolution_clock::now()).time_since_epoch().count();
}

uint64_t ExecutionTimer::StartTime() const
{
    return chrono::time_point_cast<chrono::milliseconds>(m_StartPoint).time_since_epoch().count();
}

uint64_t ExecutionTimer::SinceStart() const
{
    return Now() - StartTime();
}
