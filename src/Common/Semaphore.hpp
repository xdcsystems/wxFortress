#pragma once

#include <mutex>

static std::atomic<bool> s_pause = false;

class Semaphore
{
    public:
        std::cv_status pauseWorker()
        {
            std::unique_lock<std::mutex> lock( m_mutex );
            if ( s_pause.load() ) // already paused
            {
                lock.unlock();
                return std::cv_status::no_timeout;
            }
            s_pause.store( true );

            std::cv_status retval = m_cv2.wait_for( lock, std::chrono::milliseconds( 100 ) );

            if ( retval == std::cv_status::timeout )
                s_pause.store( false ); // not paused

            lock.unlock();
            return retval;
        }

        void checkWorkerPaused( bool value = false )
        {
            std::unique_lock<std::mutex> lock( m_mutex );

            if ( value )
                s_pause.store( true );

            if ( s_pause.load() )
            {
                m_cv2.notify_all();
                m_cv1.wait( lock, []() { return s_pause.load() == false; } );
            }
            lock.unlock();
        }

        void resumeWorker()
        {
            std::scoped_lock lock( m_mutex );
            s_pause.store( false );
            m_cv1.notify_all();
        }

    protected:
        mutable std::mutex m_mutex; // mutable allows const objects to be locked

        std::condition_variable m_cv1;
        std::condition_variable m_cv2;
};
