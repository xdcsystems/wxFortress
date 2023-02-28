// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWidgets headers)
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include "../Tools.h"
#include "Base.h"
#include "Brick.h"
#include "Bricks.h"

using namespace Shapes;

void Bricks::loadLevel( unsigned short level )
{
    const auto bricks = std::move( Tools::Instance().loadLevelFromFile( wxT( "/../resources/levels.txt" ), level ) );

    const int rows = bricks.size();
    const int cols = bricks[ 0 ].size();
    const int startRow = 12;

    m_bricks.clear();

    // let's throw some bricks
    m_bricks.reserve( rows * cols );
    for ( int row = 0; row < rows; ++row )
        for ( int col = 0; col < cols; ++col )
            m_bricks.push_back(
                std::make_shared<Brick>(
                    col,
                    row + startRow,
                    m_bricksMap.at( static_cast< BrickType >( bricks[ row ][ col ] ) ) ) );
}

void Bricks::render( bool bRun, wxDC &dc, const std::function<bool( BrickPtr )> &checkIntersects ) const
{
    bool directionSwitched = false;

    for ( const auto& brick : m_bricks )
    {
        if ( !brick->isAlive() )
            continue;
        
        if ( bRun && !directionSwitched )
            directionSwitched = checkIntersects( brick );

        brick->draw( dc );
    }
}
