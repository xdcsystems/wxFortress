#pragma once

#include <map>
#include <memory>

#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

#include "Common/NonCopyable.hpp"

// Forward declarations
class Texture2D;
class SpriteRenderer;

class CountDown : private NonCopyable
{
    using rendererPtr = std::shared_ptr<SpriteRenderer>;
    using texture2DPtr = std::shared_ptr<Texture2D>;

    public:
        CountDown();
        ~CountDown() = default;

        void resize( const wxSize& size );
        void show( const rendererPtr& renderer, unsigned char count ) const;

    private:
        texture2DPtr m_countdownSprite;
        std::map<unsigned char, glm::vec4> m_uvRects;
        constexpr inline static glm::vec2 s_countdownSize { 250.f, 250.f };
        glm::vec2 m_countDownPosition { .0f, .0f };
};
