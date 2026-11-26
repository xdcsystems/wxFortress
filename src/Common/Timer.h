#pragma once

//////////////////////////////////////////////////////////////////////////////
// Timer.h
// =======
// High Resolution Monotonic Timer based on std::chrono::steady_clock.
// Measures elapsed time with microsecond accuracy across Windows, Linux, and Unix.
//
//////////////////////////////////////////////////////////////////////////////

#include <chrono>

class Timer
{
    public:
        explicit Timer( bool initialState = true );

        void start();                       // start timer
        void stop();                        // stop the timer
        double getElapsedTime();            // get elapsed time in seconds
        double getElapsedTimeInSec();       // get elapsed time in seconds (same as getElapsedTime)
        double getElapsedTimeInMilliSec();  // get elapsed time in milliseconds
        double getElapsedTimeInMicroSec();  // get elapsed time in microseconds
        bool isStopped() const
        {
            return m_stopped;
        }

    private:
        using Clock = std::chrono::steady_clock;

        Clock::time_point m_startTime;
        Clock::time_point m_endTime;
        bool m_stopped;
};
