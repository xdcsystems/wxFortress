#pragma once

#include <memory>
#include <glm/vec3.hpp>

class SpriteRenderer;

namespace Shapes
{
    class Ball final : public Base
    {
        public:
            Ball();

            float radius() const { return m_radius; }
            void drawTinted( const std::shared_ptr<SpriteRenderer>& renderer, const glm::vec3& color ) const;

        private:
            float m_radius { 0 };
    };
}
