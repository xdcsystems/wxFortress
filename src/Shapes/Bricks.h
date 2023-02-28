#pragma once

#include <map>

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

        using BrickPtr = std::shared_ptr<Brick>;
        using BallPtr = std::shared_ptr<Base>;

        static inline const std::map<BrickType, const std::wstring> m_bricksMap =
        {
            { BrickType::NONE, wxT("") },
            { BrickType::AQUA, wxT( "brick_aqua.png" ) },
            { BrickType::RED, wxT( "brick_red.png" ) },
            { BrickType::YELLOW, wxT( "brick_yellow.png" ) },
            { BrickType::GREEN, wxT( "brick_green.png" ) },
            { BrickType::ORANGE, wxT( "brick_orange.png" ) },
            { BrickType::PURPLE, wxT( "brick_purple.png" ) },
            { BrickType::BLUE, wxT( "brick_blue.png" ) },
        };

        public:
            void loadLevel( unsigned short level = 1 );
            void render( bool bRun, wxDC& dc, const std::function<bool( BrickPtr )> &checkIntersects ) const;
            bool empty() const
            {
                return std::find_if( m_bricks.begin(), m_bricks.end(), []( BrickPtr brick ) { return brick->isAlive(); } ) == m_bricks.end();
            };

        private:
            std::vector< BrickPtr> m_bricks;
    };
}