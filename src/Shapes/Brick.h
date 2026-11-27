#pragma once

#include <memory>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include "Common/NonCopyable.hpp"

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

    class Brick : public Base, private NonCopyable
    {
        public:
            Brick( const glm::vec2& position, BrickType type, unsigned short stage = 0 );
            ~Brick() = default;

            BrickType type() const noexcept { return m_type; }
            bool isAlive() const noexcept { return m_alive; }
            unsigned char hp() const noexcept { return m_hp; }
            unsigned char maxHp() const noexcept { return m_maxHp; }
            bool isDamaged() const noexcept { return m_alive && m_hp > 0u && m_hp < m_maxHp; }

            // Applies damage; returns true if the brick was destroyed, false if it survived.
            bool takeDamage( unsigned char amount = 1u ) noexcept;
            void stepVisual( float dtSec ) noexcept;
            glm::vec3 renderTint() const noexcept;
            void kill() noexcept { m_hp = 0u; m_alive = false; m_damageFlashSec = 0.0f; }

            static glm::vec2 SizeOf( BrickType type )
            {
                if ( type == BrickType::HALF )
                    return { s_defaultSize.x / 2.0f, s_defaultSize.y };

                return s_defaultSize;
            }

        private:
            static inline const glm::vec2 s_defaultSize { 57.f, 27.f };
            BrickType m_type { BrickType::NONE };
            bool m_alive { true };
            unsigned char m_hp { 1u };
            unsigned char m_maxHp { 1u };
            float m_damageFlashSec { 0.0f };
    };

    using brickPtr = std::shared_ptr<Brick>;
}
