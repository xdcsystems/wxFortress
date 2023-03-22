#pragma once

// Forward declarations
class Texture2D;
class SpriteRenderer;

class Overlay
{
        using rendererPtr = std::shared_ptr<SpriteRenderer>;
        using texture2DPtr = std::shared_ptr<Texture2D>;

        static inline const std::map<unsigned char, const std::string> s_countdownMap =
        {
            { 3, "3.png" },
            { 2, "2.png" },
            { 1, "1.png" },
        };

        static inline std::map<unsigned char, texture2DPtr> s_countdownSprites;
 
    public:
        Overlay( const wxSize& size );

        void showPause( const rendererPtr &renderer );
        void showCountDown( const rendererPtr &renderer, unsigned char count );

    private:
        texture2DPtr m_pauseTex;
        
        const wxFont m_font;
        const wxSize m_size;
};