#pragma once

#include <AL/al.h>
#include "Common/defs.h"

#if defined( _DEBUG ) && !defined( SKIP_AL_CHECKS )
//OpenAL error checking
#define OpenAL_ErrorCheck( message )                                                              \
    {                                                                                             \
        ALenum error = alGetError();                                                              \
        if ( error != AL_NO_ERROR )                                                               \
        {                                                                                         \
            std::cerr << "OpenAL Error: " << error << " with call for " << #message << std::endl; \
        }                                                                                         \
    }
#define alec( FUNCTION_CALL ) \
    FUNCTION_CALL;            \
    OpenAL_ErrorCheck( FUNCTION_CALL )
#else
#define alec( stmt ) stmt
#endif

struct ALCdevice;
struct ALCcontext;

template <class T>
class AudioFile;

class SoundManager final
{
    public:
        SoundManager();
        ~SoundManager();

        int init();
        void playSiren();
        void playCountdown();
        void playRocket() const;
        void playDestroyBrick();
        void playPing();
        void playPong();
        void playLevelComplete();
        void playBallLost();
        void playCharShow();

    private:
        int loadBuffer( AudioFile<float>& soundFile, const std::string& fileName, ALuint* buffer );
        ALuint getFreeStereoSource() const;
        void cleanSources();
        void playStereoSource( ALuint buffer, float gain = 1.f, bool playAsync = true );

        ALCdevice* m_device { nullptr };
        ALCcontext* m_context { nullptr };
        ALuint m_monoSoundBuffer { 0 };

        ALuint m_stereoLounchSoundBuffer { 0 };
        ALuint m_stereoCountdownSoundBuffer { 0 };
        ALuint m_stereoDesroyBrickSoundBuffer { 0 };
        ALuint m_stereoPingSoundBuffer { 0 };
        ALuint m_stereoPongSoundBuffer { 0 };
        ALuint m_stereoLevelCompleteSoundBuffer { 0 };
        ALuint m_stereoBallLostSoundBuffer { 0 };
        ALuint m_stereoCharShowSoundBuffer { 0 };

        ALuint m_monoSource { 0 };
        ALuint m_stereoSource[250] {};
};
