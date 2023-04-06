#include <algorithm>
#include <iostream>

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
    while ( 1 );

    int ret( 0 ), pret( 0 );
    auto current_pts = nanoseconds::zero();
    SwsContextPtr swsCtx;
    AVFramePtr frame { av_frame_alloc() };

    if ( !frame )
        return AVERROR( ENOMEM );

    while ( !m_movie.m_quit.load( std::memory_order_relaxed ) )
    {
        size_t write_idx = m_pictQWrite.load( std::memory_order_relaxed );
        Picture *vp = &m_pictQ[write_idx];

        // Retrieve video frame.
        while ( ( ret = avcodec_receive_frame( m_codecCtx.get(), frame.get() ) ) == AVERROR( EAGAIN ) &&
                  ( pret = m_packets.sendTo( m_codecCtx.get() ) ) != AVErrorEOF ) {}

        if ( frame->format == Movie::s_hwPixFormat )
        {
            AVFramePtr sw_frame { av_frame_alloc() };
            AVFramePtr tmp_frame { av_frame_alloc() };
            
            if ( !sw_frame || !tmp_frame )
                return AVERROR( ENOMEM );

            // retrieve data from GPU to CPU
            if ( av_hwframe_transfer_data( sw_frame.get(), frame.get(), 0 ) < 0 )
                return -1;

            if ( !swsCtx )
            {
                swsCtx.reset( sws_getContext(
                    m_codecCtx->width,
                    m_codecCtx->height,
                    ( AVPixelFormat )sw_frame->format,
                    m_codecCtx->width,
                    m_codecCtx->height,
                    AV_PIX_FMT_YUV420P,
                    SWS_FAST_BILINEAR,
                    nullptr,
                    nullptr,
                    nullptr ) );

                if ( !swsCtx )
                    return -1;
            }

            av_frame_copy_props( tmp_frame.get(), frame.get() );
            tmp_frame->format = AV_PIX_FMT_YUV420P;
            tmp_frame->width = sw_frame->width;
            tmp_frame->height = sw_frame->height;
            tmp_frame->colorspace = sw_frame->colorspace;

            if ( av_frame_get_buffer( tmp_frame.get(), 0 ) != 0 )
                return AVERROR( ENOMEM );

            if ( sws_scale( swsCtx.get(),
                sw_frame->data,
                sw_frame->linesize,
                0,
                sw_frame->height,
                tmp_frame->data,
                tmp_frame->linesize ) < 0 )
                return -1;

            vp->frame.swap( tmp_frame );
        }
        else
            vp->frame.swap( frame );

        if ( ret != 0 )
        {
            if ( ret == AVErrorEOF || pret == AVErrorEOF )
                break;

            std::cerr << "Fail to receive frame: " << ret << std::endl;
            continue;
        }

        AVFrame* decoded_frame = vp->frame.get();

        // Update pts
        if ( decoded_frame->best_effort_timestamp != AVNoPtsValue )
        {
            current_pts = duration_cast<nanoseconds>(
                    seconds_d64 { av_q2d( m_stream->time_base ) * static_cast<double>( decoded_frame->best_effort_timestamp ) } );
        }
        vp->pts = current_pts;

        // Update the video clock to the next expected pts.
        auto frame_delay = av_q2d( m_codecCtx->time_base );
        frame_delay += decoded_frame->repeat_pict * ( frame_delay * 0.5 );
        current_pts += duration_cast<nanoseconds>( seconds_d64 { frame_delay } );

        // Put the frame in the queue to be loaded into a texture and displayed
        // by the rendering thread.
        write_idx = ( write_idx + 1 ) % m_pictQ.size();
        m_pictQWrite.store( write_idx, std::memory_order_release );

        // Send a packet for next receive.
        m_packets.sendTo( m_codecCtx.get() );

        if ( write_idx == m_pictQRead.load( std::memory_order_acquire ) )
        {
            // Wait until we have space
            std::unique_lock<std::mutex> lck( m_pictQMutex );
            while ( write_idx == m_pictQRead.load( std::memory_order_acquire ) && !m_movie.m_quit.load( std::memory_order_relaxed ) )
            {
                m_pictQCond.wait( lck );
            }
        }
    }

    return std::min( ret, 0 );
}

std::pair<AVFrame *, int64_t> Video::currentFrame()
{
    size_t read_idx = m_pictQRead.load( std::memory_order_relaxed );
    Picture *vp = &m_pictQ[read_idx];

    auto clocktime = m_movie.getMasterClock();
    size_t current_idx = read_idx;
    while ( 1 )
    {
        size_t next_idx = ( current_idx + 1 ) % m_pictQ.size();

        if ( next_idx == m_pictQWrite.load( std::memory_order_acquire ) )
            break;
        
        Picture *nextvp = &m_pictQ[next_idx];
        if ( clocktime < nextvp->pts )
            break;

        current_idx = next_idx;
    }

    if ( m_movie.m_quit.load( std::memory_order_relaxed ) )
        return { nullptr, 0 };

    if ( current_idx != read_idx )
    {
        vp = &m_pictQ[current_idx];
        m_pictQRead.store( current_idx, std::memory_order_release );
        std::unique_lock<std::mutex> { m_pictQMutex }.unlock();
        m_pictQCond.notify_one();
    }

    if ( vp->pts == nanoseconds::min() )
        return { nullptr, 0 };

    if ( current_idx != read_idx )
    {
        std::lock_guard<std::mutex> lck( m_dispPtsMutex );
        m_displayPts = vp->pts;
        m_displayPtsTime = get_avtime();
    }

    return { vp->frame.get(), vp->pts.count() };
}

nanoseconds Video::getClock()
{
    std::lock_guard<std::mutex> _ { m_dispPtsMutex };
    if ( m_displayPtsTime == microseconds::min() )
    {
        m_displayPtsTime = get_avtime();
        return m_displayPts;
    }
    auto delta = get_avtime() - m_displayPtsTime;
    return m_displayPts + delta;
}
