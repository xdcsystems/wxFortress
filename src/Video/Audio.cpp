#include <iostream>
#include <vector>

#include "Audio.h"
#include "Movie.h"

using std::chrono::duration_cast;

Audio::Audio( Movie &movie )
  : m_movie( movie )
{}

Audio::~Audio()
{
    cleanup();
}

void Audio::cleanup()
{
    if ( m_source )
    {
        alDeleteSources( 1, &m_source );
        m_source = 0;
    }
    if ( m_buffers[ 0 ] )
    {
        alDeleteBuffers( m_buffers.size(), m_buffers.data() );
        m_buffers = {};
    }
    if ( m_samples )
    {
        av_freep( &m_samples );
        m_samples = 0;
    }
}

int Audio::start()
{
    std::unique_lock<std::mutex> srclck( m_srcMutex, std::defer_lock );
    milliseconds sleep_time { AudioBufferTime / 3 };

    ALenum FormatStereo8 { AL_FORMAT_STEREO8 };
    ALenum FormatStereo16 { AL_FORMAT_STEREO16 };

    if ( m_codecCtx->sample_fmt == AV_SAMPLE_FMT_U8 || m_codecCtx->sample_fmt == AV_SAMPLE_FMT_U8P )
    {
        m_dstSampleFmt = AV_SAMPLE_FMT_U8;
        m_frameSize = 1;
        if ( m_codecCtx->channel_layout == AV_CH_LAYOUT_MONO )
        {
            m_dstChanLayout = m_codecCtx->channel_layout;
            m_frameSize *= 1;
            m_format = AL_FORMAT_MONO8;
        }
        if ( !m_format || m_format == -1 )
        {
            m_dstChanLayout = AV_CH_LAYOUT_STEREO;
            m_frameSize *= 2;
            m_format = FormatStereo8;
        }
    }

    if ( !m_format || m_format == -1 )
    {
        m_dstSampleFmt = AV_SAMPLE_FMT_S16;
        m_frameSize = 2;
        if ( m_codecCtx->channel_layout == AV_CH_LAYOUT_MONO )
        {
            m_dstChanLayout = m_codecCtx->channel_layout;
            m_frameSize *= 1;
            m_format = AL_FORMAT_MONO16;
        }
        if ( !m_format || m_format == -1 )
        {
            m_dstChanLayout = AV_CH_LAYOUT_STEREO;
            m_frameSize *= 2;
            m_format = FormatStereo16;
        }
    }

    void *samples { nullptr };
    ALsizei buffer_len { 0 };

    m_samples = nullptr;
    m_samplesMax = 0;
    m_samplesPos = 0;
    m_samplesLen = 0;

    m_decodedFrame.reset( av_frame_alloc() );
    if ( !m_decodedFrame )
    {
        std::cerr << "Fail to av_frame_alloc\n";
        goto finish;
    }

    if ( m_dstChanLayout )
    {
        m_swrCtx.reset( swr_alloc_set_opts( nullptr,
                m_dstChanLayout,
                m_dstSampleFmt,
                m_codecCtx->sample_rate,
                m_codecCtx->channel_layout ? m_codecCtx->channel_layout : av_get_default_channel_layout( m_codecCtx->channels ),
                m_codecCtx->sample_fmt,
                m_codecCtx->sample_rate,
                0,
                nullptr ) );
        if ( !m_swrCtx || swr_init( m_swrCtx.get() ) != 0 )
        {
            std::cerr << "Fail to initialize swr\n";
            goto finish;
        }
    }
    else
    {
        assert( 0 );
        goto finish;
    }

    alGenBuffers( m_buffers.size(), m_buffers.data() );
    alGenSources( 1, &m_source );

    if ( alGetError() != AL_NO_ERROR )
    {
        goto finish;
    }

    buffer_len = duration_cast<seconds>( m_codecCtx->sample_rate * AudioBufferTime ).count() * m_frameSize;
    samples = av_malloc( buffer_len );

    // Prefill the codec buffer.
    do
    {
        const int ret = m_packets.sendTo( m_codecCtx.get() );
        if ( ret == AVERROR( EAGAIN ) || ret == AVErrorEOF )
        {
            break;
        }
        if ( ret != 0 )
        {
            std::cout << "Fail to send packet audio: " << ret << std::endl;
        }
    }
    while ( 1 );

    srclck.lock();

    while ( !m_movie.m_quit.load( std::memory_order_relaxed ) )
    {
        ALenum state;
        ALint processed, queued;

        // First remove any processed buffers.
        alGetSourcei( m_source, AL_BUFFERS_PROCESSED, &processed );
        if ( processed > 0 )
        {
            std::vector<ALuint> bufids( processed );
            alSourceUnqueueBuffers( m_source, processed, bufids.data() );
        }

        // Refill the buffer queue.
        alGetSourcei( m_source, AL_BUFFERS_QUEUED, &queued );
        while ( queued < m_buffers.size() )
        {
            if ( !readAudio( static_cast<uint8_t *>( samples ), buffer_len ) )
            {
                break;
            }
            ALuint bufid = m_buffers[m_bufferIdx];
            m_bufferIdx = ( m_bufferIdx + 1 ) % m_buffers.size();
            alBufferData( bufid, m_format, samples, buffer_len, m_codecCtx->sample_rate );
            alSourceQueueBuffers( m_source, 1, &bufid );
            ++queued;
        }

        alGetSourcei( m_source, AL_SOURCE_STATE, &state );
        if ( state == AL_PAUSED )
        {
            alSourceRewind( m_source );
            alSourcei( m_source, AL_BUFFER, 0 );
            continue;
        }

        if ( state != AL_PLAYING && state != AL_PAUSED )
        {
            if ( !play() )
            {
                break;
            }
        }

        if ( alGetError() != AL_NO_ERROR )
        {
            return false;
        }

        m_srcCond.wait_for( srclck, sleep_time );
    }

    alSourceRewind( m_source );
    alSourcei( m_source, AL_BUFFER, 0 );

    srclck.unlock();

finish:
    av_freep( &samples );
    cleanup();

    return 0;
}

int Audio::decodeFrame()
{
    while ( !m_movie.m_quit.load( std::memory_order_relaxed ) )
    {
        int ret, pret;
        while ( ( ret = avcodec_receive_frame( m_codecCtx.get(), m_decodedFrame.get() ) ) == AVERROR( EAGAIN ) &&
                ( pret = m_packets.sendTo( m_codecCtx.get() ) ) != AVErrorEOF )
        {
        }
        if ( ret != 0 )
        {
            if ( ret == AVErrorEOF || pret == AVErrorEOF )
            {
                break;
            }
            std::cerr << "Fail to receive frame: " << ret << std::endl;
            continue;
        }
        if ( m_decodedFrame->nb_samples <= 0 )
        {
            continue;
        }
        if ( m_decodedFrame->best_effort_timestamp != AVNoPtsValue )
        {
            m_currentPts = duration_cast<nanoseconds>(
                    seconds_d64 { av_q2d( m_stream->time_base ) * static_cast<double>( m_decodedFrame->best_effort_timestamp ) } );
        }
        if ( m_decodedFrame->nb_samples > m_samplesMax )
        {
            av_freep( &m_samples );
            av_samples_alloc( &m_samples, nullptr, m_codecCtx->channels, m_decodedFrame->nb_samples, m_dstSampleFmt, 0 );
            m_samplesMax = m_decodedFrame->nb_samples;
        }
        int nsamples = swr_convert( m_swrCtx.get(),
                &m_samples,
                m_decodedFrame->nb_samples,
                (const uint8_t **)m_decodedFrame->data,
                m_decodedFrame->nb_samples );
        av_frame_unref( m_decodedFrame.get() );
        return nsamples;
    }

    return 0;
}

int Audio::readAudio( uint8_t *samples, unsigned int length )
{
    unsigned int audio_size = 0;
    length /= m_frameSize;
    while ( audio_size < length )
    {
        if ( m_samplesPos == m_samplesLen )
        {
            int frame_len = decodeFrame();
            if ( frame_len <= 0 )
            {
                break;
            }
            m_samplesLen = frame_len;
            m_samplesPos = 0;
        }

        const unsigned int len = m_samplesLen - m_samplesPos;
        unsigned int rem = length - audio_size;
        if ( rem > len )
        {
            rem = len;
        }
        std::copy_n( m_samples + m_samplesPos * m_frameSize, rem * m_frameSize, samples );
        m_samplesPos += rem;
        m_currentPts += nanoseconds { seconds { rem } } / m_codecCtx->sample_rate;
        samples += rem * m_frameSize;
        audio_size += rem;
    }

    if ( audio_size <= 0 )
    {
        return false;
    }

    if ( audio_size < length )
    {
        const unsigned int rem = length - audio_size;
        std::fill_n( samples, rem * m_frameSize, ( m_dstSampleFmt == AV_SAMPLE_FMT_U8 ? 0x80 : 0x00 ) );
        m_currentPts += nanoseconds { seconds { rem } } / m_codecCtx->sample_rate;
        audio_size += rem;
    }

    return true;
}

bool Audio::play()
{
    ALint queued {};
    alGetSourcei( m_source, AL_BUFFERS_QUEUED, &queued );
    if ( queued == 0 )
    {
        return false;
    }

    alSourcePlay( m_source );
    return true;
}

nanoseconds Audio::getClock()
{
    std::lock_guard<std::mutex> lck( m_srcMutex );
    return getClockNoLock();
}

nanoseconds Audio::getClockNoLock()
{
    nanoseconds pts = m_currentPts;

    if ( m_source )
    {
        ALint offset;
        alGetSourcei( m_source, AL_SAMPLE_OFFSET, &offset );
        ALint queued, status;
        alGetSourcei( m_source, AL_BUFFERS_QUEUED, &queued );
        alGetSourcei( m_source, AL_SOURCE_STATE, &status );

        if ( status != AL_STOPPED )
        {
            pts -= AudioBufferTime * queued;
            pts += nanoseconds { seconds { offset } } / m_codecCtx->sample_rate;
        }
    }

    return std::max( pts, nanoseconds::zero() );
}
