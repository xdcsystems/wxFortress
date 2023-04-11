#include <cassert>
#include <iostream>

#include "Movie.h"
#include "Audio.h"
#include "Chronons.h"
#include "Video.h"


static int InitDecoderHW( AVCodecContext* ctx, const enum AVHWDeviceType type )
{
    int err = 0;
    
    if ( ( err = av_hwdevice_ctx_create( &Movie::s_hwDeviceCtx, type, nullptr, nullptr, 0 ) ) < 0 )
    {
        std::cerr << "Failed to create specified HW device" << std::endl;
        return err;
    }
    ctx->hw_device_ctx = av_buffer_ref( Movie::s_hwDeviceCtx );
    if ( !ctx->hw_device_ctx )
    {
        err = AVERROR( ENOMEM );
        return err;
    }
    if ( av_hwdevice_ctx_init( ctx->hw_device_ctx ) < 0 )
    {
        std::cerr << "Error init hw codec" << std::endl;
        return -1;
    }

    return err;
}

static enum AVPixelFormat GetFormatHW( AVCodecContext* ctx, const enum AVPixelFormat* pixFormats )
{
    static const enum AVPixelFormat *s_p = nullptr;

    if ( s_p )
        return *s_p;

    for ( s_p = pixFormats; *s_p != -1; ++s_p )
    {
        if ( *s_p == Movie::s_hwPixFormat )
            return *s_p;
    }

    std::cerr << "Failed to get HW surface format" << std::endl;
    return AV_PIX_FMT_NONE;
}

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

int64_t Movie::duration()
{
    if ( m_fmtCtx && m_fmtCtx->duration != AV_NOPTS_VALUE )
    {
        return std::chrono::duration_cast<nanoseconds>(
            seconds_d64 { m_fmtCtx->duration / AV_TIME_BASE } ).count();
    }
    return -1;
}

int Movie::streamComponentOpen( unsigned int streamIndex )
{
    auto *codecpar = m_fmtCtx->streams[ streamIndex ]->codecpar;

    // find decoder for the stream
    auto *decoder = avcodec_find_decoder( codecpar->codec_id );
    if ( !decoder )
    {
        std::cerr << "Failed to find "
            << av_get_media_type_string( codecpar->codec_type ) << " codec"
            << std::endl;
        return AVERROR( EINVAL );
    }

    if ( codecpar->codec_type == AVMEDIA_TYPE_VIDEO && codecpar->codec_id == AV_CODEC_ID_H264 )
    {
        for ( int i = 0;; i++ )
        {
            const AVCodecHWConfig* config = avcodec_get_hw_config( decoder, i );
            if ( !config )
                break;

            if ( config->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX &&
                config->device_type == AV_HWDEVICE_TYPE_DXVA2 )
            {
                s_hwPixFormat = config->pix_fmt;
                break;
            }
        }
    }

    // Allocate a codec context for the decoder
    AVCodecContextPtr avctx { avcodec_alloc_context3( decoder ) };
    if ( !avctx )
    {
        std::cerr << "Failed to allocate the "
            << av_get_media_type_string( codecpar->codec_type ) << " codec context"
            << std::endl;
        return -1;
    }

    // Copy codec parameters from input stream to output codec context
    int ret = avcodec_parameters_to_context( avctx.get(), codecpar );
    if ( ret < 0 )
    {
        std::cerr << "Failed to copy "
            << av_get_media_type_string( codecpar->codec_type )
            << " codec parameters to decoder context"
            << std::endl;
        return ret;
    }

    if ( avctx->codec_type == AVMEDIA_TYPE_VIDEO && avctx->codec_id == AV_CODEC_ID_H264 )
    {
        avctx->pix_fmt = AV_PIX_FMT_YUV420P; // https://habr.com/ru/companies/intel/articles/575632/
        avctx->get_format = GetFormatHW;

        // try to use hw decoder
        if ( InitDecoderHW( avctx.get(), AV_HWDEVICE_TYPE_DXVA2 ) < 0 )
        {
            std::cerr << "Failed to init hw decoder for "
                << av_get_media_type_string( codecpar->codec_type )
                << " codec, will use sw decoder"
                << std::endl;
        }
    }

    // set codec to automatically determine how many threads suits best for the decoding job
    avctx->thread_count = 0;

    if ( decoder->capabilities | AV_CODEC_CAP_FRAME_THREADS )
        avctx->thread_type = FF_THREAD_FRAME;
    else if ( decoder->capabilities | AV_CODEC_CAP_SLICE_THREADS )
        avctx->thread_type = FF_THREAD_SLICE;
    else
        avctx->thread_count = 1; //don't use multithreading

    // Init the decoders
    if ( ( ret = avcodec_open2( avctx.get(), decoder, nullptr ) ) < 0 )
    {
        std::cerr << "Failed to open "
            << av_get_media_type_string( codecpar->codec_type )
            << " codec"
            << std::endl;

        return ret;
    }

    switch ( avctx->codec_type )
    {
        case AVMEDIA_TYPE_AUDIO:
        {
            m_audio.m_stream = m_fmtCtx->streams[ streamIndex ];
            m_audio.m_codecCtx = std::move( avctx );
        }
        break;

        case AVMEDIA_TYPE_VIDEO:
        {
            m_video.m_stream = m_fmtCtx->streams[ streamIndex ];
            m_video.m_codecCtx = std::move( avctx );
        }
        break;

        default:
        return -1;
    }
    
    return static_cast<int>( streamIndex );
}

int Movie::start()
{
    AVFormatContext *fmtctx = avformat_alloc_context();
    AVIOInterruptCB intrcb { nullptr, this };
    fmtctx->interrupt_callback = intrcb;
    if ( avformat_open_input( &fmtctx, m_filename.c_str(), nullptr, nullptr ) != 0 )
    {
        std::cerr << "Fail to avformat_open_input" << std::endl;
        return -1;
    }
    m_fmtCtx.reset( fmtctx );

    if ( avformat_find_stream_info( m_fmtCtx.get(), nullptr ) < 0 )
    {
        std::cerr << "Fail to avformat_find_stream_info" << std::endl;
        return -1;
    }

    int videoIndex { -1 };
    int audioIndex { -1 };

    for ( unsigned int i = 0; i < m_fmtCtx->nb_streams; i++ )
    {
        auto *codecpar = m_fmtCtx->streams[ i ]->codecpar;

        int ret = av_find_best_stream( m_fmtCtx.get(), codecpar->codec_type, -1, -1, nullptr, 0 );
        if ( ret < 0 )
        {
            std::cerr << "Could not find "
                << av_get_media_type_string( codecpar->codec_type ) << " stream in input file'"
                << std::endl;
            return ret;
        }

        if ( codecpar->codec_type == AVMEDIA_TYPE_VIDEO && videoIndex == -1 )
        {
            videoIndex = streamComponentOpen( ret );
        }
        else if ( codecpar->codec_type == AVMEDIA_TYPE_AUDIO && audioIndex == -1 )
        {
            audioIndex = streamComponentOpen( ret );
        }
    }

    if ( videoIndex < 0 && audioIndex < 0 )
    {
        std::cerr << "Fail to open codecs" << std::endl;
        return -1;
    }

    m_clockBase = get_avtime() + milliseconds( 750 );
    m_sync = ( audioIndex > 0 ) ? SyncMaster::Audio : SyncMaster::Video;

    auto &audioQueue = m_audio.m_packets;
    auto &videoQueue = m_video.m_packets;

    if ( audioIndex >= 0 )
    {
        m_audioThread = std::thread( &Audio::start, &m_audio );
    }
    if ( videoIndex >= 0 )
    {
        m_videoThread = std::thread( &Video::start, &m_video );
    }

    while ( !m_quit.load( std::memory_order_relaxed ) )
    {
        AVPacket packet;
        if ( av_read_frame( m_fmtCtx.get(), &packet ) < 0 )
            break;

        if ( packet.stream_index == videoIndex )
        {
            while ( !m_quit.load( std::memory_order_acquire ) && !videoQueue.put( &packet ) )
            {
                std::this_thread::sleep_for( milliseconds( 100 ) );
            }
        }
        else if ( packet.stream_index == audioIndex )
        {
            while ( !m_quit.load( std::memory_order_acquire ) && !audioQueue.put( &packet ) )
            {
                std::this_thread::sleep_for( milliseconds( 100 ) );
            }
        }

        av_packet_unref( &packet );
    }

    if ( m_video.m_codecCtx )
    {
        videoQueue.setFinished();
    }
    if ( m_audio.m_codecCtx )
    {
        audioQueue.setFinished();
    }

    if ( m_audioThread.joinable() )
    {
        m_audioThread.join();
    }
    if ( m_videoThread.joinable() )
    {
        m_videoThread.join();
    }

    m_quit.store( true );

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
