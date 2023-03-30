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
        Overlay( const wxSize& size );

        void showPause( const rendererPtr &renderer );
        void showCountDown( const rendererPtr &renderer, unsigned char count );

    private:
        texture2DPtr m_pauseTex;
        std::shared_ptr<CountDown> m_countdown;

        const wxFont m_font;
        const wxSize m_size;
};