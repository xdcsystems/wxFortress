// For compilers that support precompilation, includes "wx/wx.h".
#include <wx/wxprec.h>

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWidgets headers)
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include <iostream>
#include <mutex>

#include <AL/al.h>
#include <AL/alc.h>

#include "AudioFile/AudioFile.h"
#include "Common/Tools.h"
#include "SoundManager.h"

SoundManager::~SoundManager()
{
    // clean up our resources!
    cleanSources();

    alec( alDeleteSources( 1, &m_monoSource ) );
    alec( alDeleteSources( 250, m_stereoSource ) );
    alec( alDeleteBuffers( 1, &m_monoSoundBuffer ) );
    alec( alDeleteBuffers( 1, &m_stereoLounchSoundBuffer ) );
    alec( alDeleteBuffers( 1, &m_stereoCountdownSoundBuffer ) );
    alec( alDeleteBuffers( 1, &m_stereoDesroyBrickSoundBuffer ) );
    alec( alDeleteBuffers( 1, &m_stereoPingSoundBuffer ) );
    alec( alDeleteBuffers( 1, &m_stereoPongSoundBuffer ) );
    alec( alDeleteBuffers( 1, &m_stereoLevelCompleteSoundBuffer ) );
    alec( alDeleteBuffers( 1, &m_stereoBallLostSoundBuffer ) );
    alec( alDeleteBuffers( 1, &m_stereoCharShowSoundBuffer ) );

    alcMakeContextCurrent( nullptr );
    alcDestroyContext( m_context );
    alcCloseDevice( m_device );
}

int SoundManager::loadBuffer( AudioFile<float>& soundFile, const std::string& fileName, ALuint* buffer )
{
    std::vector<uint8_t> dataBytesPCM;

    if ( !soundFile.load( Tools::Instance().getFullFileName( "/../resources/sounds/" + fileName ) ) )
    {
        std::cerr << "failed to load sound file" << std::endl;
        return -1;
    }

    const auto convertFileToOpenALFormat = []( const AudioFile<float>& audioFile ) {
        int bitDepth = audioFile.getBitDepth();
        if ( bitDepth == 16 )
        {
            return audioFile.isStereo() ? AL_FORMAT_STEREO16 : AL_FORMAT_MONO16;
        }

        if ( bitDepth == 8 )
        {
            return audioFile.isStereo() ? AL_FORMAT_STEREO8 : AL_FORMAT_MONO8;
        }

        return -1;  // this shouldn't happen!
    };

    soundFile.writePCMToBuffer( dataBytesPCM );  //remember, we added this function to the AudioFile library

    alec( alGenBuffers( 1, buffer ) );
    alec( alBufferData( *buffer,
            convertFileToOpenALFormat( soundFile ),
            dataBytesPCM.data(),
            dataBytesPCM.size(),
            soundFile.getSampleRate() ) );

    return 0;
}

int SoundManager::init()
{
    static std::once_flag once;
    std::call_once( once, [ this ]() {
        // find the default audio device
        const ALCchar* defaultDeviceString = alcGetString( /*device*/ nullptr, ALC_DEFAULT_DEVICE_SPECIFIER );
        m_device = alcOpenDevice( defaultDeviceString );
        if ( !m_device )
        {
            std::cerr << "failed to get the default device for OpenAL" << std::endl;
            return -1;
        }
        std::cout << "OpenAL Device: " << alcGetString( m_device, ALC_DEVICE_SPECIFIER ) << std::endl;
        //OpenAL_ErrorCheck(device);

        // Create an OpenAL audio context from the device
        m_context = alcCreateContext( m_device, /*attrlist*/ nullptr );
        //OpenAL_ErrorCheck(context);

        // Activate this context so that OpenAL state modifications are applied to the context
        if ( !alcMakeContextCurrent( m_context ) )
        {
            std::cerr << "failed to make the OpenAL context the current context" << std::endl;
            return -1;
        }
        //OpenAL_ErrorCheck("Make context current");

        // Create a listener in 3d space (ie the player); (there always exists as listener, you just configure data on it)
        alec( alListener3f( AL_POSITION, 0.f, 0.f, 0.f ) );
        alec( alListener3f( AL_VELOCITY, 0.f, 0.f, 0.f ) );
        ALfloat forwardAndUpVectors[] = { /*forward = */ 1.f,
                0.f,
                0.f,
                /* up = */ 0.f,
                1.f,
                0.f };
        alec( alListenerfv( AL_ORIENTATION, forwardAndUpVectors ) );

        // Create buffers that hold our sound data; these are shared between contexts and ar defined at a device level
        AudioFile<float> soundFile;
        loadBuffer( soundFile, "Rocket.wav", &m_monoSoundBuffer );

        // load a stereo files into a buffers
        loadBuffer( soundFile, "Launch.wav", &m_stereoLounchSoundBuffer );
        loadBuffer( soundFile, "Countdown.wav", &m_stereoCountdownSoundBuffer );
        loadBuffer( soundFile, "DesroyBrick.wav", &m_stereoDesroyBrickSoundBuffer );
        loadBuffer( soundFile, "Ping.wav", &m_stereoPingSoundBuffer );
        loadBuffer( soundFile, "Pong.wav", &m_stereoPongSoundBuffer );
        loadBuffer( soundFile, "LevelComplete.wav", &m_stereoLevelCompleteSoundBuffer );
        loadBuffer( soundFile, "BallLost.wav", &m_stereoBallLostSoundBuffer );
        loadBuffer( soundFile, "Char.wav", &m_stereoCharShowSoundBuffer );

        // create a sound source for our stereo sound; note 3d positioning doesn't work with stereo files because
        // stereo files are typically used for music. stereo files come out of both ears so it is hard to know
        // what the sound should be doing based on 3d position data.
        alec( alGenSources( 250, m_stereoSource ) );

        // create a sound source that play's our mono sound (from the sound buffer)
        alec( alGenSources( 1, &m_monoSource ) );
        alec( alSource3f( m_monoSource, AL_POSITION, 1.f, 0.f, 0.f ) );
        alec( alSource3f( m_monoSource, AL_VELOCITY, 0.f, 0.f, 0.f ) );
        alec( alSourcef( m_monoSource, AL_PITCH, 1.f ) );
        alec( alSourcef( m_monoSource, AL_GAIN, 1.f ) );
        alec( alSourcei( m_monoSource, AL_LOOPING, AL_FALSE ) );
        alec( alSourcei( m_monoSource, AL_BUFFER, m_monoSoundBuffer ) );
    } );
    return 0;
}

ALuint SoundManager::getFreeStereoSource() const
{
    ALint sourceState( 0 );
    for ( const auto source : m_stereoSource )
    {
        alec( alGetSourcei( source, AL_SOURCE_STATE, &sourceState ) );
        if ( sourceState == AL_PLAYING )
            continue;

        return source;
    }
    return 0;
}

void SoundManager::cleanSources()
{
    for ( const auto source : m_stereoSource )
    {
        ALint processed( 0 );
        alGetSourcei( source, AL_BUFFERS_PROCESSED, &processed );

        while ( processed-- )
        {
            ALuint buffer( 0 );
            alSourceUnqueueBuffers( source, 1, &buffer );
        }
    }
}

void SoundManager::playStereoSource( ALuint buffer, float gain, bool playAsync )
{
    // Find free source
    auto source = getFreeStereoSource();
    if ( source == 0 )               // no free source
    {
        source = m_stereoSource[0];  // hope for good luck.
    }

    ////alec(alSource3f(stereoSource, AL_POSITION, 0.f, 0.f, 1.f)); //NOTE: this does not work like mono sound positions!
    ////alec(alSource3f(stereoSource, AL_VELOCITY, 0.f, 0.f, 0.f));
    alec( alSourcef( source, AL_PITCH, 1.f ) );
    alec( alSourcef( source, AL_GAIN, gain ) );
    alec( alSourcei( source, AL_LOOPING, AL_FALSE ) );
    alec( alSourcei( source, AL_BUFFER, buffer ) );

    alec( alSourcePlay( source ) );

    ALint processed( 0 );
    alec( alGetSourcei( source, AL_BUFFERS_PROCESSED, &processed ) );

    //bool active = true;
    while ( processed-- )
    {
        ALuint buffer( 0 );
        alec( alSourceUnqueueBuffers( source, 1, &buffer ) );
        // ReportError();

        // active = RefillBuffer( buffer );

        // alSourceQueueBuffers( soruce, 1, &buffer );
        // ReportError();
    }

    if ( playAsync )
        return;

    // play the stereo sound source sync
    alec( alSourcePlay( source ) );
    ALint sourceState( 0 );
    alec( alGetSourcei( source, AL_SOURCE_STATE, &sourceState ) );
    while ( sourceState == AL_PLAYING )
    {
        //basically loop until we're done playing the current sound source
        alec( alGetSourcei( source, AL_SOURCE_STATE, &sourceState ) );
        wxYield();
    }
}

void SoundManager::playRocket() const
{
    alec( alSourcePlay( m_monoSource ) );
}

void SoundManager::playDestroyBrick()
{
    alec( alSourceStop( m_monoSource ) );
    playStereoSource( m_stereoDesroyBrickSoundBuffer );
}

void SoundManager::playPing()
{
    //alec( alSourcei( m_stereoSource, AL_BUFFER, m_stereoPingSoundBuffer ) );
    playStereoSource( m_stereoPongSoundBuffer, 0.2f );
}

void SoundManager::playPong()
{
    playStereoSource( m_stereoPongSoundBuffer, 0.25f );
}

void SoundManager::playSiren()
{
    playStereoSource( m_stereoLounchSoundBuffer, 0.5f, false );
}

void SoundManager::playCountdown()
{
    playStereoSource( m_stereoCountdownSoundBuffer, 0.5f, false );
}

void SoundManager::playLevelComplete()
{
    playStereoSource( m_stereoLevelCompleteSoundBuffer, 0.5f );
}

void SoundManager::playBallLost()
{
    playStereoSource( m_stereoBallLostSoundBuffer );
}

void SoundManager::playCharShow()
{
    playStereoSource( m_stereoCharShowSoundBuffer, 1.f );  // TODO play async
}