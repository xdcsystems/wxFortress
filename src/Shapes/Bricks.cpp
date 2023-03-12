// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWidgets headers)
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include "Common/Tools.h"
#include "Renderer/ResourceManager.h"
#include "Renderer/SpriteRenderer.h"
#include "Base.h"
#include "Brick.h"
#include "Bricks.h"

using namespace Shapes;

Bricks::Bricks()
{
    for ( const auto& brick : s_bricksMap )
    {
        if ( brick.second.empty() )
        {
            s_brickSprites[ brick.first ] = nullptr;
            continue;
        }

        s_brickSprites[ brick.first ] = ResourceManager::LoadTexture(
            "/../resources/images/Bricks/" + brick.second,
            true,
            "brick_" + static_cast< int >( brick.first )
        );
    }
}

void Bricks::loadLevel( unsigned short level )
{
    const auto bricks = std::move( Tools::Instance().loadLevelFromFile( "/../resources/levels.txt", level ) );

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
                    s_brickSprites.at( static_cast< BrickType >( bricks[ row ][ col ] ) ) ) );
}

void Bricks::render( bool bRun, rendererPtr renderer, const std::function<bool( brickPtr )>& checkIntersects ) const
{
    bool directionSwitched = false;

    renderer->selectShader();

    for ( const auto& brick : m_bricks )
    {
        if ( !brick->isAlive() )
            continue;

        if ( bRun && !directionSwitched )
            directionSwitched = checkIntersects( brick );

        brick->draw( renderer );
    }
}
