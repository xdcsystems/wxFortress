#pragma once

#include <map>

class SpriteRenderer;

namespace Shapes
{
    class Bricks
    {
        enum class BrickType
        {
            NONE,
            AQUA,
            RED,
            YELLOW,
            GREEN,
            ORANGE,
            PURPLE,
            BLUE,
        };

        using brickPtr = std::shared_ptr<Brick>;
        using ballPtr = std::shared_ptr<Base>;
        using rendererPtr = std::shared_ptr<SpriteRenderer>;
        using texture2DPtr = std::shared_ptr<Texture2D>;

        static inline const std::map<BrickType, const std::string> s_bricksMap =
        {
            { BrickType::NONE, "" },
            { BrickType::AQUA, "brick_aqua.png" },
            { BrickType::RED, "brick_red.png" },
            { BrickType::YELLOW, "brick_yellow.png" },
            { BrickType::GREEN, "brick_green.png" },
            { BrickType::ORANGE, "brick_orange.png" },
            { BrickType::PURPLE, "brick_purple.png" },
            { BrickType::BLUE, "brick_blue.png" },
        };

        static inline std::map<BrickType, texture2DPtr> s_brickSprites;

        public:
            Bricks();

            void loadLevel( unsigned short level = 1 );
            void render( bool bRun, const std::function<bool( brickPtr )>& checkIntersects ) const;
            bool empty() const {
                return std::find_if( m_bricks.begin(), m_bricks.end(), []( brickPtr brick ) { return brick->isAlive(); } ) == m_bricks.end();
            };
            void paint( rendererPtr renderer );


        private:
            std::vector< brickPtr> m_bricks;
    };
}