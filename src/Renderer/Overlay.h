#pragma once

// Forward declarations
class Texture2D;
class SpriteRenderer;
class CountDown;

class Overlay
{
        using rendererPtr = std::shared_ptr<SpriteRenderer>;
        using texture2DPtr = std::shared_ptr<Texture2D>;

    public:
        Overlay();

        void resize( const wxSize& size );
        void showPause( const rendererPtr &renderer ) const;
        void showCountDown( const rendererPtr &renderer, unsigned char count ) const;

    private:
        texture2DPtr m_pauseTex;
        std::shared_ptr<CountDown> m_countdown;

        wxSize m_size;
};