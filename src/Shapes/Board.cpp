// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWidgets headers)
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include "Common/xRect.hpp"
#include "Renderer/ResourceManager.h"
#include "Base.h"
#include "Board.h"

using namespace Shapes;

Board::Board()
 : Base()
{
    load( ResourceManager::LoadTexture( "/../resources/images/SimpleBoard.png", true, "simple_board" ) );
}

xRect Board::admissibleBounds( const xRect& bounds ) const
{
    auto boundsRect = std::move( Base::bounds() );
    boundsRect.Inset( bounds.m_width / 2, 0.0 );
    
    return boundsRect;
}
