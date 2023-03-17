#pragma once

//////////////////////////////////////////////////////////////////////////////
// Timer.h
// =======
// High Resolution Timer.
// This timer is able to measure the elapsed time with 1 micro-second accuracy
// in both Windows, Linux and Unix system 
//
//////////////////////////////////////////////////////////////////////////////

#if defined(WIN32) || defined(_WIN32)   // Windows system specific
#include <windows.h>
#else          // Unix based system specific
#include <sys/time.h>
#endif


class Timer
{
    public:
        Timer( bool initialState = true );    

        void   start();                             // start timer
        void   stop();                              // stop the timer
        double getElapsedTime();                    // get elapsed time in second
        double getElapsedTimeInSec();               // get elapsed time in second (same as getElapsedTime)
        double getElapsedTimeInMilliSec();          // get elapsed time in milli-second
        double getElapsedTimeInMicroSec();          // get elapsed time in micro-second
        bool    isStopped() const { return stopped; }

    protected:

    private:
        double startTimeInMicroSec;                 // starting time in micro-second
        double endTimeInMicroSec;                   // ending time in micro-second
        bool    stopped;                             // stop flag 
    #if defined(WIN32) || defined(_WIN32)
        LARGE_INTEGER frequency;                    // ticks per second
        LARGE_INTEGER startCount;                   //
        LARGE_INTEGER endCount;                     //
    #else
        timeval startCount;                         //
        timeval endCount;                           //
    #endif
};
