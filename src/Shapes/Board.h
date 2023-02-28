#pragma once

namespace Shapes
{
    class Board : public Base
    {
        public:
            Board();
            wxRect admissibleBounds( const wxRect &bounds ) const;
    };
}