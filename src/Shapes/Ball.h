#pragma once

namespace Shapes
{
    class Ball final : public Base
    {
        public:
            Ball();

            float radius() const { return m_radius; }

        private:
            float m_radius { 0 };
    };
}
