// For compilers that support precompilation, includes "wx/wx.h".
#include <wx/wxprec.h>

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWidgets headers)
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include <memory>

#include <wx/dc.h>
#include <wx/dcbuffer.h>

#include "Common/defs.h"
#include "Common/Tools.h"
#include "Common/Rect.hpp"

#include "Counter.hpp"
#include "Panel.h"

using namespace ControlPanel;

// clang-format off
BEGIN_EVENT_TABLE( Panel, wxWindow )
    EVT_PAINT( Panel::onPaint )
END_EVENT_TABLE()
// clang-format on

Panel::Panel( wxWindow *parent, wxWindowID id, const wxPoint &pos, const wxSize &size, long style, const wxString &name )
  : wxWindow( parent, id, pos, size, style, name )
{
    Hide();
    init();
}

void Panel::init()
{
    SetBackgroundStyle( wxBG_STYLE_PAINT );
    SetBackgroundColour( *wxBLACK );

    m_bitmapPanelBuffer = Tools::Instance().loadBitmap( "resources/images/ControlPanel/panel.png" );
    m_bitmapPanelLaunchedBuffer = Tools::Instance().loadBitmap( "resources/images/ControlPanel/panel_launched.png" );
    m_bitmapControlBgBuffer = Tools::Instance().loadBitmap( "resources/images/ControlPanel/control/background.png" );
    m_numbers = Tools::Instance().loadBitmap( "resources/images/ControlPanel/control/numbers.png" );

    // hi score counter
    // rectangle( 67, 181, 78, 21 );
    m_hiScore = std::make_shared< Counter<unsigned long> >( 67, 181 );

    // score
    // rectangle( 67, 267, 78, 21 );
    m_score = std::make_shared< Counter<unsigned long> >( 67, 267 );

    // lives
    // rectangle( 67, 353, 78, 21 );
    m_lives = std::make_shared< Counter<unsigned char> >( 67, 353, INITIAL_LIVES_VALUE );

    // level
    // rectangle( 67, 439, 78, 21 );
    m_level = std::make_shared< Counter<unsigned short> >( 67, 439 );
}

void Panel::activate()
{
    refreshDisplayValue( m_hiScore );
    refreshDisplayValue( m_score );
    refreshDisplayValue( m_lives );
    refreshDisplayValue( m_level );
    Update();
}

void Panel::onPaint( wxPaintEvent & )
{
    wxPaintDC dc( this );
    render( dc, m_bitmapPanelBuffer );
}

void Panel::paintLaunched()
{
    wxClientDC dc( this );
    render( dc, m_bitmapPanelLaunchedBuffer );
}

void Panel::render( wxDC &dc, const bitmapPtr &panel )
{
    const auto &clientSize = GetClientSize();
    if ( !m_mdc )
    {
        if ( !panel || clientSize.x < 1 || clientSize.y < 1 )
            return;

        m_mdc = std::make_shared<wxMemoryDC>();
        m_mdc->SetBrush( *wxBLACK_BRUSH );
    }

    if ( !m_numbersDC )
    {
        if ( !m_numbers )
            return;

        m_numbersDC = std::make_shared<wxMemoryDC>();
        m_numbersDC->SelectObject( *m_numbers );
    }

    m_mdc->SelectObject( *panel );

    dc.Blit( 0, 0, clientSize.x, clientSize.y, m_mdc.get(), 0, 0 );

    m_mdc->SelectObject( wxNullBitmap );
}

template <typename T>
void Panel::refreshDisplayValue( counterPtr<T> counter )
{
    m_mdc->SelectObject( *m_bitmapPanelBuffer );

    counter->clear( m_bitmapControlBgBuffer, *m_mdc );

    counter->render( m_mdc, m_numbersDC );

    m_mdc->SelectObject( wxNullBitmap );

    Refresh();
}

void Panel::increaseScore()
{
    m_score->increaseValue();
    refreshDisplayValue( m_score );
}

unsigned short Panel::currentLevel() const
{
    return m_level->value();
}

unsigned short Panel::increaseLevel()
{
    m_level->increaseValue();
    m_hiScore->reset( m_hiScore->value() + m_score->value() );
    m_score->reset();

    if ( IsShown() )
    {
        activate();
    }

    return m_level->value();
}

unsigned char Panel::decreaseLives()
{
    m_lives->decreaseValue();
    refreshDisplayValue( m_lives );

    return m_lives->value();
}

void Panel::reset()
{
    m_lives->reset( INITIAL_LIVES_VALUE );
    m_level->reset();
    m_score->reset();
}
