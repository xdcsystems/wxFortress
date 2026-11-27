// For compilers that support precompilation, includes "wx/wx.h".
#include <wx/wxprec.h>

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWidgets headers)
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include <map>

#include "Common/Tools.h"
#include "ResourceManager.h"
#include "Texture.h"
#include "SpriteRenderer.h"
#include "CountDown.h"


CountDown::CountDown()
{
    m_countdownSprite = ResourceManager::LoadTexture(
        "resources/images/CountDown.png",
        "countdown" );

    if ( m_countdownSprite && m_countdownSprite->Width > 0 )
    {
        const float divisionScale = 1.f / static_cast<float>( m_countdownSprite->Width );
        for ( unsigned char count = 1; count <= 3; ++count )
        {
            const float left = divisionScale * static_cast<float>( count - 1 ) * s_countdownSize.x;
            const float right = left + divisionScale * s_countdownSize.x;

            m_countdownMap[ count ] = SpriteRenderer::generateBuffer( {
                // pos      // tex
                0.f, 0.f,    left,  1.f,
                0.f, 1.f,    left,  0.f,
                1.f, 0.f,    right, 1.f,
                1.f, 1.f,    right, 0.f,
            } );
        }
    }
}

CountDown::~CountDown()
{
    clear();
}

void CountDown::clear()
{
    for ( auto& countdown : m_countdownMap )
    {
        SpriteRenderer::clearBuffer( countdown.second );
        countdown.second = 0;
    }
}

void CountDown::resize( const wxSize& size )
{
    const glm::vec2 screenSize( static_cast<float>( size.x ), static_cast<float>( size.y ) );
    m_countDownPosition = ( screenSize - s_countdownSize ) * 0.5f;
}

void CountDown::show( const rendererPtr& renderer, unsigned char count ) const
{
    if ( !m_countdownSprite )
        return;

    const auto it = m_countdownMap.find( count );
    if ( it == m_countdownMap.end() || it->second == 0 )
        return;

    m_countdownSprite->bind();
    renderer->drawSprite(
        it->second,
        m_countDownPosition,
        s_countdownSize );
}
