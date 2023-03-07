#pragma once

namespace Shapes
{
    class Brick : public Base
    {
        using texture2DPtr = std::shared_ptr<Texture2D>;
        using rendererPtr = std::shared_ptr<SpriteRenderer>;

        public:
            Brick( double x, double y, texture2DPtr sprite );

            bool isAlive() const { return m_alive; }
            void kill() { m_alive = false; }

        private:
            static inline const glm::vec2 s_defaultSize = { 57.f, 27.f };
            bool m_alive = true;
    };
    
    using brickPtr = std::shared_ptr<Brick>;
}