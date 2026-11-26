//////////////////////////////////////////////////////////////////////////////
// Timer.cpp
// =========
// High Resolution Monotonic Timer based on std::chrono::steady_clock.
//
//////////////////////////////////////////////////////////////////////////////

#include "Timer.h"

#include <algorithm>

Timer::Timer( bool initialState )
  : m_startTime( Clock::now() )
  , m_endTime( m_startTime )
  , m_stopped( initialState )
{
}

void Timer::start()
{
    m_stopped = false;
    m_startTime = Clock::now();
    m_endTime = m_startTime;
}

void Timer::stop()
{
    if ( !m_stopped )
    {
        m_stopped = true;
        m_endTime = Clock::now();
    }
}

double Timer::getElapsedTimeInMicroSec()
{
    if ( !m_stopped )
    {
        m_endTime = Clock::now();
    }

    const auto delta = std::chrono::duration<double, std::micro>( m_endTime - m_startTime ).count();
    return std::max( 0.0, delta );
}

double Timer::getElapsedTimeInMilliSec()
{
    return getElapsedTimeInMicroSec() * 0.001;
}

double Timer::getElapsedTimeInSec()
{
    return getElapsedTimeInMicroSec() * 0.000001;
}

double Timer::getElapsedTime()
{
    return getElapsedTimeInSec();
}
