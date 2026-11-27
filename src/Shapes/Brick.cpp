// For compilers that support precompilation, includes "wx/wx.h".
#include <wx/wxprec.h>

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWidgets headers)
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include <algorithm>
#include <cmath>

#include "Base.h"
#include "Brick.h"
#include "BrickMechanics.hpp"

using namespace Shapes;

Brick::Brick( const glm::vec2& position, BrickType type, unsigned short stage )
    : m_type( type )
{
    m_size = SizeOf( type );
    m_position = position;
    m_maxHp = BrickMechanics::initialHpForType( static_cast<unsigned char>( type ), stage );
    m_hp = m_maxHp;
    m_alive = ( m_hp > 0u );
}

bool Brick::takeDamage( unsigned char amount ) noexcept
{
    if ( !m_alive || amount == 0u )
        return false;

    if ( m_hp > amount )
    {
        m_hp = static_cast<unsigned char>( m_hp - amount );
        m_damageFlashSec = BrickMechanics::kArmorFlashDurationSec;
        return false;
    }

    kill();
    return true;
}

void Brick::stepVisual( float dtSec ) noexcept
{
    if ( m_damageFlashSec <= 0.0f || !std::isfinite( dtSec ) || dtSec <= 0.0f )
        return;
    m_damageFlashSec = ( std::max )( 0.0f, m_damageFlashSec - dtSec );
}

glm::vec3 Brick::renderTint() const noexcept
{
    return BrickMechanics::damageTint( m_hp, m_maxHp, m_damageFlashSec );
}
