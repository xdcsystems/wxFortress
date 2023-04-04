#include <cassert>
#include <iostream>

#include "Movie.h"
#include "Audio.h"
#include "Chronons.h"
#include "Video.h"

static AVBufferRef* hw_device_ctx = NULL;

Movie::Movie()
  : m_audio( *this )
  , m_video( *this )
{}

Movie::~Movie()
{
    if ( m_parseThread.joinable() )
    {
        m_parseThread.join();
    }
}

int Movie::open( std::string filename )
{
    if ( m_parseThread.joinable() )
    {
        return -1;
    }
    m_filename = std::move( filename );
    m_parseThread = std::thread( &Movie::start, this );

    return 0;
}

void Movie::close()
{
    m_quit = true;
    m_video.m_pictQCond.notify_one();
    if ( m_parseThread.joinable() )
    {
        m_parseThread.join();
    }
}

std::pair<AVFrame *, int64_t> Movie::currentFrame()
{
    return m_video.currentFrame();
}

static int hw_decoder_init( AVCodecContext* ctx, const enum AVHWDeviceType type )
{
    int err = 0;

    if ( ( err = av_hwdevice_ctx_create( &hw_device_ctx, type, NULL, NULL, 0 ) ) < 0 )
    {
        fprintf( stderr, "Failed to create specified HW device.\n" );
        return err;
    }
    ctx->hw_device_ctx = av_buffer_ref( hw_device_ctx );

    return err;
}

static enum AVPixelFormat get_hw_format( AVCodecContext* ctx, const enum AVPixelFormat* pix_fmts )
{
    const enum AVPixelFormat* p;

    for ( p = pix_fmts; *p != -1; p++ )
    {
        if ( *p == Movie::s_hwPixFormat )
            return *p;
    }

    fprintf( stderr, "Failed to get HW surface format.\n" );
    return AV_PIX_FMT_NONE;
}

int Movie::streamComponentOpen( unsigned int stream_index )
{
    AVCodecContextPtr avctx { avcodec_alloc_context3( nullptr ) };
    if ( !avctx )
    {
        std::cerr << "Fail to avcodec_alloc_context3\n";
        return -1;
    }

    int ret;

    ret = avcodec_parameters_to_context( avctx.get(), m_fmtCtx->streams[stream_index]->codecpar );
    if ( ret < 0 )
    {
        std::cerr << "Fail to avcodec_parameters_to_context\n";
        return -1;
    }

    AVCodec* codec { avcodec_find_decoder( avctx->codec_id ) };

    if ( avctx->codec_type == AVMEDIA_TYPE_VIDEO && avctx->codec_id == AV_CODEC_ID_H264 )
    {
        avctx->pix_fmt = AV_PIX_FMT_YUV420P;

        // try to use hw decoder
        if ( hw_decoder_init( avctx.get(), AV_HWDEVICE_TYPE_DXVA2 ) == 0 )
        {
            avctx->get_format = get_hw_format;

            for ( int i = 0;; i++ )
            {
                const AVCodecHWConfig* config = avcodec_get_hw_config( codec, i );
                if ( !config )
                    goto Next;

                if ( config->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX &&
                    config->device_type == AV_HWDEVICE_TYPE_DXVA2 )
                {
                    s_hwPixFormat = config->pix_fmt;
                    break;
                }
            }
        }
    }

Next:
    if ( !codec || avcodec_open2( avctx.get(), codec, nullptr ) < 0 )
    {
        std::cerr << "Unsupported codec: " << avcodec_get_name( avctx->codec_id ) << std::endl;
        return -1;
    }

    switch ( avctx->codec_type )
    {
        case AVMEDIA_TYPE_AUDIO :
            m_audio.m_stream = m_fmtCtx->streams[stream_index];
            m_audio.m_codecCtx = std::move( avctx );
        break;

        case AVMEDIA_TYPE_VIDEO :
        {
            m_video.m_stream = m_fmtCtx->streams[ stream_index ];
            m_video.m_codecCtx = std::move( avctx );
        }
        break;

        default :
            return -1;
    }

    return static_cast<int>( stream_index );
}

int Movie::start()
{
    AVFormatContext *fmtctx = avformat_alloc_context();
    AVIOInterruptCB intrcb { nullptr, this };
    fmtctx->interrupt_callback = intrcb;
    if ( avformat_open_input( &fmtctx, m_filename.c_str(), nullptr, nullptr ) != 0 )
    {
        std::cerr << "Fail to avformat_open_input\n";
        return -1;
    }
    m_fmtCtx.reset( fmtctx );

    if ( avformat_find_stream_info( m_fmtCtx.get(), nullptr ) < 0 )
    {
        std::cerr << "Fail to avformat_find_stream_info\n";
        return -1;
    }

    int video_index { -1 };
    int audio_index { -1 };

    for ( unsigned int i = 0; i < m_fmtCtx->nb_streams; i++ )
    {
        auto *codecpar = m_fmtCtx->streams[i]->codecpar;
        if ( codecpar->codec_type == AVMEDIA_TYPE_VIDEO )
        {
            video_index = streamComponentOpen( i );
        }
        else if ( codecpar->codec_type == AVMEDIA_TYPE_AUDIO )
        {
            audio_index = streamComponentOpen( i );
        }
    }

    if ( video_index < 0 && audio_index < 0 )
    {
        std::cerr << "Fail to open codecs\n";
        return -1;
    }

    m_clockBase = get_avtime() + milliseconds( 750 );
    m_sync = ( audio_index > 0 ) ? SyncMaster::Audio : SyncMaster::Video;

    auto &audio_queue = m_audio.m_packets;
    auto &video_queue = m_video.m_packets;

    if ( audio_index >= 0 )
    {
        m_audioThread = std::thread( &Audio::start, &m_audio );
    }
    if ( video_index >= 0 )
    {
        m_videoThread = std::thread( &Video::start, &m_video );
    }

    while ( !m_quit.load( std::memory_order_relaxed ) )
    {
        AVPacket packet;
        if ( av_read_frame( m_fmtCtx.get(), &packet ) < 0 )
            break;

        if ( packet.stream_index == video_index )
        {
            while ( !m_quit.load( std::memory_order_acquire ) && !video_queue.put( &packet ) )
            {
                std::this_thread::sleep_for( milliseconds( 100 ) );
            }
        }
        else if ( packet.stream_index == audio_index )
        {
            while ( !m_quit.load( std::memory_order_acquire ) && !audio_queue.put( &packet ) )
            {
                std::this_thread::sleep_for( milliseconds( 100 ) );
            }
        }

        av_packet_unref( &packet );
    }

    if ( m_video.m_codecCtx )
    {
        video_queue.setFinished();
    }
    if ( m_audio.m_codecCtx )
    {
        audio_queue.setFinished();
    }

    if ( m_audioThread.joinable() )
    {
        m_audioThread.join();
    }
    if ( m_videoThread.joinable() )
    {
        m_videoThread.join();
    }

    return 0;
}

nanoseconds Movie::getMasterClock()
{
    switch ( m_sync )
    {
        case SyncMaster::Audio :
            return m_audio.getClock();
        case SyncMaster::Video :
            return m_video.getClock();
        default :
            assert( 0 );
            return getClock();
    }
}

nanoseconds Movie::getClock()
{
    assert( 0 );
    return nanoseconds::min();
}
