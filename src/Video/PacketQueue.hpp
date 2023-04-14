#pragma once

#include <deque>
#include <mutex>
#include <thread>
#include <cassert>

#include "FFmpeg.h"

template <size_t SizeLimit>
class PacketQueue
{
    public:
        PacketQueue() = default;
        ~PacketQueue()
        {
            for ( AVPacket &pkt : m_packets )
            {
                av_packet_unref( &pkt );
            }
            m_packets.clear();
            m_totalSize = 0;
        }

        int sendTo( AVCodecContext *codecctx )
        {
            std::unique_lock<std::mutex> lck( m_mutex );

            AVPacket *pkt = getPacket( lck );
            int ret = avcodec_send_packet( codecctx, pkt );

            if ( !pkt )
            {
                if ( !ret )
                    return AVErrorEOF;

                return ret;
            }

            if ( ret != AVERROR( EAGAIN ) )
            {
                pop();
            }

            return ret;
        }

        void setFinished()
        {
            {
                std::lock_guard<std::mutex> lck( m_mutex );
                m_finished = true;
            }
            m_cv.notify_one();
        }

        bool put( AVPacket &pkt )
        {
            {
                std::lock_guard<std::mutex> lck( m_mutex );

                if ( m_totalSize >= SizeLimit )
                    return false;

                m_packets.emplace_back( std::move( pkt ) );
                m_totalSize += static_cast<size_t>( m_packets.back().size );
            }
            m_cv.notify_one();
            return true;
        }

    private:
        AVPacket *getPacket( std::unique_lock<std::mutex> &lck )
        {
            while ( m_packets.empty() && !m_finished )
            {
                m_cv.wait( lck );
            }
            return m_packets.empty() ? nullptr : &m_packets.front();
        }

        void pop()
        {
            AVPacket *pkt = &m_packets.front();
            m_totalSize -= static_cast<size_t>( pkt->size );
            av_packet_unref( pkt );
            m_packets.pop_front();
        }

        std::mutex m_mutex;
        std::condition_variable m_cv;
        std::deque<AVPacket> m_packets;
        size_t m_totalSize { 0 };
        bool m_finished { false };
};
