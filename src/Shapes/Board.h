#pragma once

namespace Shapes
{
    class Board : public Base
    {
        public:
            Board();
            wxRect2DDouble admissibleBounds( const wxRect2DDouble& bounds ) const;
    };
}