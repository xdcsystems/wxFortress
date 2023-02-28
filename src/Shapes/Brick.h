#pragma once

namespace Shapes
{
    class Brick : public Base
    {
        public:
            Brick( int x, int y, const std::wstring &filename );
            void draw( wxDC& dc );
            bool isAlive() const { return m_alive; }
            void kill( wxDC& dc );

        private:
            static const wxSize s_defaultSize;
            bool m_painted = false;
            bool m_alive = true;
    };
}