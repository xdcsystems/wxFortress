// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWidgets headers)
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include "Common/Tools.h"
#include "Common/xRect.hpp"
#include "Renderer/Texture.h"
#include "Renderer/ResourceManager.h"
#include "Renderer/SpriteRenderer.h"
#include "Base.h"
#include "Brick.h"
#include "Bricks.h"

using namespace Shapes;

Bricks::Bricks()
{
    m_bricksSprite = ResourceManager::LoadTexture(
        "/../resources/images/Bricks/bricks.png",
        true,
        "bricks" );
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
   
    const glm::vec2 textureSize = { m_bricksSprite->Width, m_bricksSprite->Height };

    for ( int row = 0; row < rows; ++row )
        for ( int col = 0; col < cols; ++col )
            m_bricks.push_back( 
                std::make_shared<Brick>(
                    col,
                    row + startRow,
                    static_cast< BrickType >( bricks[ row ][ col ] ),
                    textureSize
                ) );
}

void Bricks::render( bool bRun, const std::function<bool( brickPtr )>& checkIntersects ) const
{
    bool hasContact = false;

    for ( const auto& brick : m_bricks )
    {
        if ( !brick->isAlive() )
            continue;

        if ( bRun && !hasContact )
        {
            hasContact = checkIntersects( brick );
            if ( hasContact )
                brick->kill();
        }
    }
}

void Bricks::draw( rendererPtr renderer )
{
    m_bricksSprite->bind();
    
    for ( const auto& brick : m_bricks )
        if ( brick->isAlive() )
            renderer->drawSprite( brick->VBO(), brick->position(), brick->size() );
}
