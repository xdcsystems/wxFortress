#include <algorithm>
#include <iostream>
#include <atomic>
#include <condition_variable>

#include "Video.h"
#include "Movie.h"

using std::chrono::duration_cast;

Video::Video( Movie &movie )
  : m_movie( movie )
{}

int Video::start()
{
    std::for_each( m_pictQ.begin(), m_pictQ.end(), []( Picture &pic ) {
        pic.frame = AVFramePtr { av_frame_alloc() };
        pic.pts = nanoseconds::min();
    } );

    // Prefill the codec buffer.
    do
    {
        int ret = m_packets.sendTo( m_codecCtx.get() );

        if ( ret == AVERROR( EAGAIN ) || ret == AVErrorEOF )
            break;

        if ( ret != 0 )
        {
            std::cout << "Fail to send packet video: " << ret << std::endl;
        }
    }
    while ( true );

    int ret( 0 );
    int pret( 0 );
    auto currentPts = nanoseconds::zero();

    AVFramePtr frame { av_frame_alloc() };
    if ( !frame )
        return AVERROR( ENOMEM );

    while ( !m_movie.m_quit.load( std::memory_order_relaxed ) )
    {
        size_t writeIdx = m_pictQWrite.load( std::memory_order_relaxed );
        Picture *vp = &m_pictQ[ writeIdx ];

        // Retrieve video frame.
        while ( ( ret = avcodec_receive_frame( m_codecCtx.get(), frame.get() ) ) == AVERROR( EAGAIN ) &&
                  ( pret = m_packets.sendTo( m_codecCtx.get() ) ) != AVErrorEOF ) {}

        if ( ret != 0 )
        {
            if ( ret == AVErrorEOF || pret == AVErrorEOF )
                break;

            std::cerr << "Fail to receive frame: " << ret << std::endl;
            continue;
        }

        if ( frame->format == Movie::s_hwPixFormat )
        {
            // retrieve data from GPU to CPU
            if ( av_hwframe_transfer_data( vp->frame.get(), frame.get(), 0 ) < 0 )
                return -1;

            av_frame_copy_props( vp->frame.get(), frame.get() );
        }
        else
            vp->frame.swap( frame );

        AVFrame* decodedFrame = vp->frame.get();

        // Update pts
        if ( decodedFrame->best_effort_timestamp != AVNoPtsValue )
        {
            currentPts = duration_cast<nanoseconds>(
                    seconds_d64 { av_q2d( m_stream->time_base ) * static_cast<double>( decodedFrame->best_effort_timestamp ) } );
        }
        vp->pts = currentPts;

        // Update the video clock to the next expected pts.
        auto frameDelay = av_q2d( m_codecCtx->time_base );
        frameDelay += decodedFrame->repeat_pict * ( frameDelay * 0.5 );
        currentPts += duration_cast<nanoseconds>( seconds_d64 { frameDelay } );

        // Put the frame in the queue to be loaded into a texture and displayed
        // by the rendering thread.
        writeIdx = ( writeIdx + 1 ) % m_pictQ.size();
        m_pictQWrite.store( writeIdx, std::memory_order_release );

        // Send a packet for next receive.
        m_packets.sendTo( m_codecCtx.get() );

        if ( writeIdx == m_pictQRead.load( std::memory_order_acquire ) )
        {
            // Wait until we have space
            std::unique_lock<std::mutex> lock { m_pictQMutex };
            while ( writeIdx == m_pictQRead.load( std::memory_order_acquire ) && !m_movie.m_quit.load( std::memory_order_relaxed ) )
            {
                m_pictQCond.wait( lock );
            }
        }
    }

    return std::min( ret, 0 );
}

std::pair<AVFrame *, int64_t> Video::currentFrame()
{
    size_t readIdx = m_pictQRead.load( std::memory_order_relaxed );
    Picture *vp = &m_pictQ[ readIdx ];

    auto clocktime = m_movie.getMasterClock();
    size_t currentIdx = readIdx;
    while ( true )
    {
        size_t nextIdx = ( currentIdx + 1 ) % m_pictQ.size();

        if ( nextIdx == m_pictQWrite.load( std::memory_order_acquire ) )
            break;
        
        Picture *nextvp = &m_pictQ[nextIdx];
        if ( clocktime < nextvp->pts )
            break;

        currentIdx = nextIdx;
    }

    if ( m_movie.m_quit.load( std::memory_order_relaxed ) )
        return { nullptr, 0 };

    if ( currentIdx != readIdx )
    {
        vp = &m_pictQ[ currentIdx ];
        m_pictQRead.store( currentIdx, std::memory_order_release );
        std::unique_lock<std::mutex> { m_pictQMutex }.unlock();
        m_pictQCond.notify_one();
    }

    if ( vp->pts == nanoseconds::min() )
        return { nullptr, 0 };

    if ( currentIdx != readIdx )
    {
        std::unique_lock<std::mutex> lock { m_dispPtsMutex };
        m_displayPts = vp->pts;
        m_displayPtsTime = get_avtime();
    }

    return { vp->frame.get(), vp->pts.count() };
}

nanoseconds Video::getClock()
{
    std::unique_lock<std::mutex> lock { m_dispPtsMutex };
    if ( m_displayPtsTime == microseconds::min() )
    {
        m_displayPtsTime = get_avtime();
        return m_displayPts;
    }
    auto delta = get_avtime() - m_displayPtsTime;
    return m_displayPts + delta;
}
