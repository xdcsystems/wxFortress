#pragma once

#include <condition_variable>
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
            std::unique_lock<std::mutex> lock { m_mutex };

            AVPacket *pkt = getPacket( lock );
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
                const bool hasSpace = m_totalSize < SizeLimit;
                lock.unlock();
                if ( hasSpace )
                    m_spaceCv.notify_one();
                return ret;
            }

            return ret;
        }

        void setFinished()
        {
            std::unique_lock<std::mutex> lock { m_mutex };
            m_finished = true;
            lock.unlock();
            
            m_cv.notify_all();
            m_spaceCv.notify_all();
        }

        void abort()
        {
            std::unique_lock<std::mutex> lock { m_mutex };
            m_aborted = true;
            m_finished = true;
            lock.unlock();

            m_cv.notify_all();
            m_spaceCv.notify_all();
        }

        bool putBlocking( AVPacket &pkt )
        {
            std::unique_lock<std::mutex> lock { m_mutex };
            m_spaceCv.wait( lock, [ this ]() {
                return m_totalSize < SizeLimit || m_aborted || m_finished;
            } );

            if ( m_aborted || m_finished )
                return false;

            m_packets.emplace_back( std::move( pkt ) );
            m_totalSize += static_cast<size_t>( m_packets.back().size );

            lock.unlock();

            m_cv.notify_one();

            return true;
        }

        bool put( AVPacket &pkt )
        {
            std::unique_lock<std::mutex> lock { m_mutex };

            if ( m_totalSize >= SizeLimit )
                return false;

            m_packets.emplace_back( std::move( pkt ) );
            m_totalSize += static_cast<size_t>( m_packets.back().size );
            
            lock.unlock();

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
        std::condition_variable m_spaceCv;
        std::deque<AVPacket> m_packets;
        size_t m_totalSize { 0 };
        bool m_finished { false };
        bool m_aborted { false };
};
