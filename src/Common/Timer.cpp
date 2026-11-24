//////////////////////////////////////////////////////////////////////////////
// Timer.cpp
// =========
// High Resolution Timer.
// This timer is able to measure the elapsed time with 1 micro-second accuracy
// in both Windows, Linux and Unix system
//
//////////////////////////////////////////////////////////////////////////////

#include "Timer.h"
#include <cstdlib>

Timer::Timer( bool initialState )
  : m_stopped( initialState )
{
#if defined( WIN32 ) || defined( _WIN32 )
    QueryPerformanceFrequency( &m_frequency );
    m_startCount.QuadPart = 0;
    m_endCount.QuadPart = 0;
#else
    m_startCount.tv_sec = m_startCount.tv_usec = 0;
    m_endCount.tv_sec = m_endCount.tv_usec = 0;
#endif
    m_startTimeInMicroSec = 0;
    m_endTimeInMicroSec = 0;
}

///////////////////////////////////////////////////////////////////////////////
// start timer.
// startCount will be set at this point.
///////////////////////////////////////////////////////////////////////////////
void Timer::start()
{
    m_stopped = false;  // reset stop flag

#if defined( WIN32 ) || defined( _WIN32 )
    QueryPerformanceCounter( &m_startCount );
#else
    gettimeofday( &m_startCount, nullptr );
#endif
}

///////////////////////////////////////////////////////////////////////////////
// stop the timer.
// endCount will be set at this point.
///////////////////////////////////////////////////////////////////////////////
void Timer::stop()
{
    m_stopped = true;  // set timer stopped flag

#if defined( WIN32 ) || defined( _WIN32 )
    QueryPerformanceCounter( &m_endCount );
#else
    gettimeofday( &m_endCount, nullptr );
#endif
}

///////////////////////////////////////////////////////////////////////////////
// compute elapsed time in micro-second resolution.
// other getElapsedTime will call this first, then convert to correspond resolution.
///////////////////////////////////////////////////////////////////////////////
double Timer::getElapsedTimeInMicroSec()
{
    double elapsedMicroSec = 0.0;

#if defined( WIN32 ) || defined( _WIN32 )
    if ( !m_stopped )
    {
        QueryPerformanceCounter( &m_endCount );
    }

    if ( m_frequency.QuadPart > 0 )
    {
        const double deltaTicks = static_cast<double>( m_endCount.QuadPart - m_startCount.QuadPart );
        elapsedMicroSec = deltaTicks * ( 1000000.0 / static_cast<double>( m_frequency.QuadPart ) );
    }
#else
    if ( !m_stopped )
        gettimeofday( &m_endCount, nullptr );

    const double deltaSec = static_cast<double>( m_endCount.tv_sec - m_startCount.tv_sec );
    const double deltaUsec = static_cast<double>( m_endCount.tv_usec - m_startCount.tv_usec );
    elapsedMicroSec = deltaSec * 1000000.0 + deltaUsec;
#endif

    m_startTimeInMicroSec = 0.0;
    m_endTimeInMicroSec = elapsedMicroSec;
    return elapsedMicroSec;
}

///////////////////////////////////////////////////////////////////////////////
// divide elapsedTimeInMicroSec by 1000
///////////////////////////////////////////////////////////////////////////////
double Timer::getElapsedTimeInMilliSec()
{
    return getElapsedTimeInMicroSec() * 0.001;
}

///////////////////////////////////////////////////////////////////////////////
// divide elapsedTimeInMicroSec by 1000000
///////////////////////////////////////////////////////////////////////////////
double Timer::getElapsedTimeInSec()
{
    return getElapsedTimeInMicroSec() * 0.000001;
}

///////////////////////////////////////////////////////////////////////////////
// same as getElapsedTimeInSec()
///////////////////////////////////////////////////////////////////////////////
double Timer::getElapsedTime()
{
    return getElapsedTimeInSec();
}
