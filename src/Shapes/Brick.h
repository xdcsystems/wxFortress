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
    };

    class Brick : public Base
    {
        public:
            Brick( float x, float y, BrickType type, const glm::vec2& texSize  );
            ~Brick();

            bool isAlive() const { return m_alive; }
            void kill() { m_alive = false; }

        private:
            static inline const glm::vec2 s_defaultSize = { 57.f, 27.f };
            bool m_alive = true;
    };
    
    using brickPtr = std::shared_ptr<Brick>;
}