#pragma once

//////////////////////////////////////////////////////////////////////////////
// Timer.h
// =======
// High Resolution Timer.
// This timer is able to measure the elapsed time with 1 micro-second accuracy
// in both Windows, Linux and Unix system
//
//////////////////////////////////////////////////////////////////////////////

#if defined( WIN32 ) || defined( _WIN32 )  // Windows system specific
#include <windows.h>
#else                                      // Unix based system specific
#include <sys/time.h>
#endif

class Timer
{
    public:
        Timer( bool initialState = true );

        void start();                       // start timer
        void stop();                        // stop the timer
        double getElapsedTime();            // get elapsed time in second
        double getElapsedTimeInSec();       // get elapsed time in second (same as getElapsedTime)
        double getElapsedTimeInMilliSec();  // get elapsed time in milli-second
        double getElapsedTimeInMicroSec();  // get elapsed time in micro-second
        bool isStopped() const
        {
            return m_stopped;
        }

    private:
        double m_startTimeInMicroSec = 0;  // starting time in micro-second
        double m_endTimeInMicroSec = 0;    // ending time in micro-second
        bool m_stopped;                    // stop flag
#if defined( WIN32 ) || defined( _WIN32 )
        LARGE_INTEGER m_frequency = {
          {0, 0}
        };  // ticks per second
        LARGE_INTEGER m_startCount = {
          {0, 0}
        };  //
        LARGE_INTEGER m_endCount = {
          {0, 0}
        };  //
#else
        timeval m_startCount = 0;  //
        timeval m_endCount = 0;    //
#endif
};
