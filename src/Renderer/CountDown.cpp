// For compilers that support precompilation, includes "wx/wx.h".
#include <wx/wxprec.h>

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWidgets headers)
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

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
        const float uScale = s_countdownSize.x / static_cast<float>( m_countdownSprite->Width );
        for ( unsigned char count = 1; count <= 3; ++count )
        {
            m_uvRects[ count ] = { static_cast<float>( count - 1 ) * uScale, 0.f, uScale, 1.f };
        }
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

    const auto it = m_uvRects.find( count );
    if ( it == m_uvRects.end() )
        return;

    m_countdownSprite->bind();
    renderer->drawSprite(
        m_countDownPosition,
        s_countdownSize,
        glm::vec3( 1.0f ),
        it->second );
}
