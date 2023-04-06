#pragma once

namespace Shapes
{
    enum class BrickType : unsigned char
    {
        NONE,
        AQUA,
        RED,
        YELLOW,
        GREEN,
        ORANGE,
        PURPLE,
        BLUE,
        HALF,
    };

    class Brick : public Base
    {
        public:
            Brick( const glm::vec2 &position, BrickType type, const glm::vec2 &texSize );
            ~Brick();

            bool isAlive() const { return m_alive; }
            void kill() { m_alive = false; }

            static glm::vec2 SizeOf( BrickType type )
            {
                if ( type == BrickType::HALF )
                    return { s_defaultSize.x /2, s_defaultSize.y };
                
                return s_defaultSize;
            }

        private:
            static inline const glm::vec2 s_defaultSize = { 57.f, 27.f };
            bool m_alive = true;
    };
    
    using brickPtr = std::shared_ptr<Brick>;
}