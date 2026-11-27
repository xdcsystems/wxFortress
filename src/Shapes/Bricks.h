#pragma once

#include <algorithm>
#include <array>
#include <functional>
#include <memory>
#include <vector>

#include <glm/vec4.hpp>

#include "Common/NonCopyable.hpp"

// Forward declarations
class Texture2D;
class SpriteRenderer;

namespace Shapes
{
    class Brick;

    class Bricks : private NonCopyable
    {
        using brickPtr = std::shared_ptr<Brick>;
        using rendererPtr = std::shared_ptr<SpriteRenderer>;
        using texture2DPtr = std::shared_ptr<Texture2D>;

        public:
            static constexpr size_t kVisibleBrickTypeCount = 7; // AQUA..BLUE

            Bricks();
            ~Bricks() = default;

            void loadLevel( unsigned short level = 1, unsigned short stage = 0 );
            void updateVisuals( float dtSec ) noexcept;
            void checkContact( const std::function<bool( brickPtr )>& checkIntersects ) const;
            const std::vector<brickPtr>& items() const { return m_bricks; }
            bool empty() const
            {
                return std::none_of( m_bricks.begin(), m_bricks.end(), []( const brickPtr& brick ) {
                    return brick->isAlive();
                } );
            }
            void draw( const rendererPtr& renderer );

        private:
            std::vector<brickPtr> m_bricks;
            texture2DPtr m_bricksSprite;
            std::array<glm::vec4, kVisibleBrickTypeCount> m_typeUvRects {};
    };
}
