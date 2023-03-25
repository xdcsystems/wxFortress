#pragma once

namespace Shapes
{
    enum class ExplosionType : unsigned char
    {
        NONE,
        BASE,
        RED,
        YELLOW,
        GREEN,
    };

    class Explosion : public Base
    {
        using basePtr = std::shared_ptr<Base>;

        public:
            Explosion( const basePtr &shape, const glm::vec2 &size );

            bool isActive() const { return m_active; }
            unsigned short current() { return m_current++; }
            void kill() { m_active = false; }

        private:
            bool m_active = true;
            unsigned short m_current = 0;
    };
    
    using explosionPtr = std::shared_ptr<Explosion>;
}