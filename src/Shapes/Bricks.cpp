// For compilers that support precompilation, includes "wx/wx.h".
#include <wx/wxprec.h>

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWidgets headers)
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include "Common/Tools.h"
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
        "resources/images/bricks.png",
        "bricks" );

    if ( m_bricksSprite && m_bricksSprite->Width > 0 )
    {
        const float divisionScale = 1.f / static_cast<float>( m_bricksSprite->Width );
        const float brickW = Brick::SizeOf( BrickType::AQUA ).x;

        for ( size_t i = 0; i < kVisibleBrickTypeCount; ++i )
        {
            const float left = divisionScale * static_cast<float>( i ) * brickW;
            const float right = left + divisionScale * brickW;

            m_typeVBOs[ i ] = SpriteRenderer::generateBuffer( {
                // pos      // tex
                0.f, 0.f,   left,  1.f,
                0.f, 1.f,   left,  0.f,
                1.f, 0.f,   right, 1.f,
                1.f, 1.f,   right, 0.f,
            } );
        }
    }
}

Bricks::~Bricks()
{
    for ( unsigned int& vbo : m_typeVBOs )
    {
        SpriteRenderer::clearBuffer( vbo );
        vbo = 0;
    }
}

void Bricks::loadLevel( unsigned short level )
{
    const auto bricks = Tools::Instance().loadLevel( "resources/levels.txt", level );

    if ( bricks.empty() )
        return;

    const size_t rows = bricks.size();
    const int startRow = 11;

    m_bricks.clear();

    size_t totalBricks = 0;
    for ( const auto& r : bricks )
        totalBricks += r.size();
    m_bricks.reserve( totalBricks );

    glm::vec2 currentPos = { .0f, .0f };

    for ( size_t row = 0; row < rows; ++row )
    {
        for ( size_t col = 0; col < bricks[ row ].size(); ++col )
        {
            const auto type = static_cast<BrickType>( bricks[ row ][ col ] );
            currentPos.y = Brick::SizeOf( type ).y * static_cast<float>( row + startRow );
            m_bricks.emplace_back( std::make_shared<Brick>( currentPos, type ) );
            currentPos.x += Brick::SizeOf( type ).x;
        }
        currentPos.x = 0.0f;
    }
}

void Bricks::checkContact( const std::function<bool( brickPtr )>& checkIntersects ) const
{
    for ( const auto& brick : m_bricks )
    {
        if ( !brick->isAlive() )
            continue;

        if ( checkIntersects( brick ) )
            break;
    }
}

void Bricks::draw( const rendererPtr& renderer )
{
    m_bricksSprite->bind();

    for ( const auto& brick : m_bricks )
    {
        if ( !brick->isAlive() )
            continue;

        const unsigned char typeVal = static_cast<unsigned char>( brick->type() );
        if ( typeVal >= 1 && typeVal <= kVisibleBrickTypeCount )
        {
            renderer->drawSprite( m_typeVBOs[ typeVal - 1 ], brick->position(), brick->size() );
        }
    }
}
