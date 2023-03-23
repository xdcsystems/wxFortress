#pragma once

namespace Shapes
{
    class Ball final : public Base
    {
        public:
            Ball();
            ContactPosition intersect( const Rect &rect ) const;

            float radius() const { return m_radius; }

        private:
            float m_radius = 0;
            float m_radiusSquared = 0;
    };
}
