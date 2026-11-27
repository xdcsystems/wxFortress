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
        const float uScale = Brick::SizeOf( BrickType::AQUA ).x / static_cast<float>( m_bricksSprite->Width );
        for ( size_t i = 0; i < kVisibleBrickTypeCount; ++i )
        {
            m_typeUvRects[ i ] = { static_cast<float>( i ) * uScale, 0.f, uScale, 1.f };
        }
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
    if ( !m_bricksSprite || m_bricksSprite->Width <= 0 )
        return;

    m_bricksSprite->bind();

    for ( const auto& brick : m_bricks )
    {
        if ( !brick->isAlive() )
            continue;

        const unsigned char typeVal = static_cast<unsigned char>( brick->type() );
        if ( typeVal >= 1 && typeVal <= kVisibleBrickTypeCount )
        {
            renderer->drawSprite( brick->position(), brick->size(), glm::vec3( 1.0f ), m_typeUvRects[ typeVal - 1 ] );
        }
    }
}
