#include "executionTimer.h"
#include <chrono>

ExecutionTimer::ExecutionTimer()
{
    m_StartPoint = Now();
}

TimePoint ExecutionTimer::Now() const
{
    return std::chrono::steady_clock::now();
}

TimePoint ExecutionTimer::StartTime() const
{
    return m_StartPoint;
}
