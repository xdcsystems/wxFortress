#pragma once

// Forward declarations
class Texture2D;
class SpriteRenderer;

class CountDown
{
    using rendererPtr = std::shared_ptr<SpriteRenderer>;
    using texture2DPtr = std::shared_ptr<Texture2D>;

    public:
        CountDown();
        ~CountDown();

        void resize( const wxSize& size );
        void show( const rendererPtr& renderer, unsigned char count ) const;

    private:
        void clear();

        texture2DPtr m_countdownSprite;

        std::map<unsigned char, unsigned int> m_countdownMap
        {
            { 3, 0 },
            { 2, 0 },
            { 1, 0 },
        };
        constexpr inline static glm::vec2 s_countdownSize { 250.f, 250.f };
        glm::vec2 m_countDownPosition { .0f, .0f };
};
