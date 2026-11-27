#pragma once

#include <cmath>
#include <cstddef>

namespace Shapes
{
    class Explosion : public Base
    {
        using basePtr = std::shared_ptr<Base>;

        public:
            static constexpr float kFramesPerSec = 60.0f;

            Explosion( const basePtr &shape, const glm::vec2 &size );

            bool isActive() const noexcept { return m_active; }
            unsigned short currentFrame() const noexcept { return static_cast<unsigned short>( m_frameAccum ); }
            void step( float dtSec, std::size_t totalFrames ) noexcept
            {
                if ( !m_active || !std::isfinite( dtSec ) || dtSec <= 0.0f )
                    return;
                m_frameAccum += dtSec * kFramesPerSec;
                if ( static_cast<std::size_t>( m_frameAccum ) >= totalFrames )
                    m_active = false;
            }
            void kill() noexcept { m_active = false; }

        private:
            bool m_active { true };
            float m_frameAccum { 0.0f };
    };
    
    using explosionPtr = std::shared_ptr<Explosion>;
}

