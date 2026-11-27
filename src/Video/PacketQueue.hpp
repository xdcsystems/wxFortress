#pragma once

#include <algorithm>
#include <cassert>
#include <condition_variable>
#include <deque>
#include <iostream>
#include <mutex>
#include <thread>

#include "FFmpeg.h"

class PacketQueue
{
    public:
        explicit PacketQueue( size_t sizeLimit )
            : m_sizeLimit( sizeLimit )
        {}
        ~PacketQueue()
        {
            m_packets.clear();
            m_totalSize = 0;
        }

        int sendTo( AVCodecContext *codecctx )
        {
            AVPacketPtr pkt;
            size_t pktSize = 0;
            bool sendFlush = false;

            {
                std::unique_lock<std::mutex> lock { m_mutex };
                m_cv.wait( lock, [ this ]() {
                    return !m_packets.empty() || m_finished || m_aborted;
                } );

                if ( m_aborted )
                    return AVErrorEOF;

                if ( m_packets.empty() )
                {
                    if ( m_flushSent )
                        return AVErrorEOF;

                    m_flushSent = true;
                    sendFlush = true;
                }
                else
                {
                    pkt = std::move( m_packets.front() );
                    pktSize = pkt ? static_cast<size_t>( std::max( pkt->size, 0 ) ) : 0;
                    m_packets.pop_front();
                    m_totalSize = ( m_totalSize >= pktSize ) ? ( m_totalSize - pktSize ) : 0;
                }
            }

            if ( sendFlush )
            {
                const int ret = avcodec_send_packet( codecctx, nullptr );
                if ( ret == AVERROR( EAGAIN ) )
                {
                    std::lock_guard<std::mutex> lock { m_mutex };
                    m_flushSent = false;
                }
                return ret;
            }

            const int ret = avcodec_send_packet( codecctx, pkt.get() );
            if ( ret == AVERROR( EAGAIN ) )
            {
                std::lock_guard<std::mutex> lock { m_mutex };
                if ( !m_aborted )
                {
                    m_packets.emplace_front( std::move( pkt ) );
                    m_totalSize += pktSize;
                }
                return ret;
            }

            m_spaceCv.notify_one();
            return ret;
        }

        void setFinished()
        {
            {
                std::lock_guard<std::mutex> lock { m_mutex };
                m_finished = true;
            }
            m_cv.notify_all();
            m_spaceCv.notify_all();
        }

        void abort()
        {
            {
                std::lock_guard<std::mutex> lock { m_mutex };
                m_aborted = true;
                m_finished = true;
            }
            m_cv.notify_all();
            m_spaceCv.notify_all();
        }

        bool putBlocking( AVPacketPtr &&pkt )
        {
            if ( !pkt )
                return false;

            const size_t pktSize = static_cast<size_t>( std::max( pkt->size, 0 ) );
            {
                std::unique_lock<std::mutex> lock { m_mutex };
                m_spaceCv.wait( lock, [ this ]() {
                    return m_totalSize < m_sizeLimit || m_aborted || m_finished;
                } );

                if ( m_aborted || m_finished )
                    return false;

                m_packets.emplace_back( std::move( pkt ) );
                m_totalSize += pktSize;
            }

            m_cv.notify_one();
            return true;
        }

        bool put( AVPacketPtr &&pkt )
        {
            if ( !pkt )
                return false;

            const size_t pktSize = static_cast<size_t>( std::max( pkt->size, 0 ) );
            {
                std::lock_guard<std::mutex> lock { m_mutex };
                if ( m_aborted || m_finished || m_totalSize >= m_sizeLimit )
                    return false;

                m_packets.emplace_back( std::move( pkt ) );
                m_totalSize += pktSize;
            }

            m_cv.notify_one();
            return true;
        }

        void prefillCodec( AVCodecContext* codecCtx, const char* streamLabel )
        {
            while ( true )
            {
                const int ret = sendTo( codecCtx );
                if ( ret == AVERROR( EAGAIN ) || ret == AVErrorEOF )
                    break;
                if ( ret != 0 )
                    std::cout << "Fail to send packet " << streamLabel << ": " << ret << std::endl;
            }
        }

        int receiveFrame( AVCodecContext* codecCtx, AVFrame* frame, int& outSendRet )
        {
            int ret = 0;
            outSendRet = 0;
            while ( ( ret = avcodec_receive_frame( codecCtx, frame ) ) == AVERROR( EAGAIN ) &&
                    ( outSendRet = sendTo( codecCtx ) ) != AVErrorEOF )
            {}
            return ret;
        }

    private:
        std::mutex m_mutex;
        std::condition_variable m_cv;
        std::condition_variable m_spaceCv;
        std::deque<AVPacketPtr> m_packets;
        size_t m_sizeLimit { 0 };
        size_t m_totalSize { 0 };
        bool m_finished { false };
        bool m_aborted { false };
        bool m_flushSent { false };
};
