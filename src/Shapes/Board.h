#pragma once

#include <memory>
#include <glm/vec3.hpp>

class SpriteRenderer;

namespace Shapes
{
    class Board : public Base
    {
        public:
            Board();

            void drawTinted( const std::shared_ptr<SpriteRenderer>& renderer, const glm::vec3& color ) const;
    };
}
