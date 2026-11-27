#pragma once

#include <memory>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

class SpriteRenderer;

namespace Shapes
{
    class Board : public Base
    {
        public:
            Board();

            void drawTinted( const std::shared_ptr<SpriteRenderer>& renderer, const glm::vec3& color ) const;
            void drawRectTinted( const std::shared_ptr<SpriteRenderer>& renderer, const glm::vec2& pos, const glm::vec2& size, const glm::vec3& color ) const;
    };
}
