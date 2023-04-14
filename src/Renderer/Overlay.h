#pragma once

// Forward declarations
class Texture2D;
class SpriteRenderer;
class CountDown;

class Overlay
{
        using rendererPtr = std::shared_ptr<SpriteRenderer>;
        using texture2DPtr = std::shared_ptr<Texture2D>;
        using countDownPtr = std::shared_ptr<CountDown>;

    public:
        Overlay( const rendererPtr& renderer );

        void resize( const wxSize& size );
        void showPause() const;
        void showCountDown( unsigned char count ) const;

    private:
        texture2DPtr m_pauseTex;
        countDownPtr m_countdown;
        rendererPtr m_renderer;

        wxSize m_size;
};