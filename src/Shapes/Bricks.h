#pragma once

// Forward declarations
class SpriteRenderer;

namespace Shapes
{
    class Bricks
    {
        using brickPtr = std::shared_ptr<Brick>;
        using ballPtr = std::shared_ptr<Base>;
        using rendererPtr = std::shared_ptr<SpriteRenderer>;
        using texture2DPtr = std::shared_ptr<Texture2D>;

        public:
            Bricks();

            void loadLevel( unsigned short level = 1 );
            void render( bool bRun, const std::function<bool( brickPtr )>& checkIntersects ) const;
            bool empty() const {
                return std::find_if( m_bricks.begin(), m_bricks.end(), []( brickPtr brick ) { return brick->isAlive(); } ) == m_bricks.end();
            };
            void draw( rendererPtr renderer );


        private:
            std::vector< brickPtr> m_bricks;
            texture2DPtr m_bricksSprite;
    };
}