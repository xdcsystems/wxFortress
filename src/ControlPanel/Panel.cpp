// For compilers that support precompilation, includes "wx/wx.h".
#include <wx/wxprec.h>

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWidgets headers)
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include <algorithm>
#include <memory>

#include <wx/dc.h>
#include <wx/dcbuffer.h>
#include <wx/ffile.h>
#include <wx/filename.h>
#include <wx/stdpaths.h>

#include "Common/defs.h"
#include "Common/Tools.h"

#include "Counter.hpp"
#include "Panel.h"

DEFINE_LOCAL_EVENT_TYPE( wxEVT_LAUNCH_CLICKED )

using namespace ControlPanel;

namespace
{
    // Red LAUNCH button on panel.png (208x729): center (104, 600), bezel radius ~36px.
    constexpr int kLaunchCenterX = 104;
    constexpr int kLaunchCenterY = 600;
    constexpr int kLaunchRadius = 36;

#ifndef WXFORTRESS_TESTING
    wxFileName hiScoreFilePath()
    {
        return wxFileName( wxStandardPaths::Get().GetUserDataDir(), wxS( "hiscore.dat" ) );
    }
#endif
} // namespace

// clang-format off
BEGIN_EVENT_TABLE( Panel, wxWindow )
    EVT_PAINT( Panel::onPaint )
    EVT_LEFT_DOWN( Panel::onLeftDown )
    EVT_MOTION( Panel::onMouseMove )
END_EVENT_TABLE()
// clang-format on

Panel::Panel( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name )
    : wxWindow( parent, id, pos, size, style, name )
{
    SetCanFocus( false );
    Hide();
    init();
}

Panel::~Panel()
{
    saveHiScoreIfDirty();
}

void Panel::init()
{
    SetBackgroundStyle( wxBG_STYLE_PAINT );
    SetBackgroundColour( *wxBLACK );

    m_bitmapPanelBuffer = Tools::Instance().loadBitmap( "resources/images/ControlPanel/panel.png" );
    m_bitmapPanelLaunchedBuffer = Tools::Instance().loadBitmap( "resources/images/ControlPanel/panel_launched.png" );
    m_bitmapControlBgBuffer = Tools::Instance().loadBitmap( "resources/images/ControlPanel/control/background.png" );
    m_numbers = Tools::Instance().loadBitmap( "resources/images/ControlPanel/control/numbers.png" );

    m_lives.reset( INITIAL_LIVES_VALUE );
    loadHiScore();
}

void Panel::loadHiScore()
{
#ifndef WXFORTRESS_TESTING
    const wxFileName path = hiScoreFilePath();
    if ( path.FileExists() )
    {
        // wxFFile uses _wfopen on Windows so Unicode/Cyrillic user profile paths work reliably.
        wxFFile file( path.GetFullPath(), wxS( "r" ) );
        wxString contents;
        if ( file.IsOpened() && file.ReadAll( &contents ) )
        {
            unsigned long long stored = 0;
            if ( contents.Trim( true ).Trim( false ).ToULongLong( &stored ) )
            {
                const uint32_t clamped = static_cast<uint32_t>(
                    ( std::min )( stored, static_cast<unsigned long long>( Counter::kMaxDisplayValue ) ) );
                m_hiScore.reset( clamped );
            }
        }
    }
#endif
    m_runStartHiScore = m_hiScore.value();
    m_newRecordNotified = false;
    m_hiScoreDirty = false;
}

void Panel::saveHiScoreIfDirty()
{
    if ( !m_hiScoreDirty )
        return;

#ifdef WXFORTRESS_TESTING
    m_hiScoreDirty = false;
#else
    const wxFileName path = hiScoreFilePath();
    const wxString dir = path.GetPath();
    if ( !dir.empty() && !wxDirExists( dir ) )
        wxFileName::Mkdir( dir, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL );

    wxFFile file( path.GetFullPath(), wxS( "w" ) );
    if ( file.IsOpened() && file.Write( wxString::Format( wxS( "%u\n" ), m_hiScore.value() ) ) )
        m_hiScoreDirty = false;
#endif
}

void Panel::activate()
{
    const bool wasLaunched = m_isLaunched;
    m_isLaunched = false;
    refreshDisplayValue( m_hiScore );
    refreshDisplayValue( m_score );
    refreshDisplayValue( m_lives );
    refreshDisplayValue( m_level );
    if ( wasLaunched )
        Refresh( false );
    Update();
}

bool Panel::isPointInLaunchButton( const wxPoint& pt ) const
{
    const int dx = pt.x - kLaunchCenterX;
    const int dy = pt.y - kLaunchCenterY;
    return ( dx * dx + dy * dy ) <= ( kLaunchRadius * kLaunchRadius );
}

void Panel::onMouseMove( wxMouseEvent& event )
{
    const bool hover = isPointInLaunchButton( event.GetPosition() );
    if ( hover != m_hoverLaunch )
    {
        m_hoverLaunch = hover;
        SetCursor( hover ? wxCursor( wxCURSOR_HAND ) : wxNullCursor );
    }
    event.Skip();
}

void Panel::onLeftDown( wxMouseEvent& event )
{
    if ( isPointInLaunchButton( event.GetPosition() ) )
    {
        if ( auto* parent = GetParent() )
        {
            parent->GetEventHandler()->QueueEvent( new wxCommandEvent( wxEVT_LAUNCH_CLICKED ) );
        }
    }
    event.Skip();
}

void Panel::onPaint( wxPaintEvent& )
{
    wxPaintDC dc( this );
    // Intentional equipment re-init effect: panel_launched.png zeroes counter windows on LAUNCH until activate() restores live values.
    render( dc, m_isLaunched ? m_bitmapPanelLaunchedBuffer : m_bitmapPanelBuffer );
}

void Panel::paintLaunched()
{
    m_isLaunched = true;
    Refresh( false );
    Update();
}

void Panel::render( wxDC& dc, const bitmapPtr& panel )
{
    const auto& clientSize = GetClientSize();
    if ( !panel || !panel->IsOk() || clientSize.x < 1 || clientSize.y < 1 )
        return;

    if ( !m_mdc )
    {
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

    const wxRect clip = wxRect( clientSize ).Intersect( wxRect( panel->GetSize() ) );
    if ( clip.IsEmpty() )
        return;

    m_mdc->SelectObject( *panel );

    bool blitted = false;
    for ( wxRegionIterator upd( GetUpdateRegion() ); upd; ++upd )
    {
        const wxRect r = upd.GetRect().Intersect( clip );
        if ( !r.IsEmpty() )
        {
            dc.Blit( r.x, r.y, r.width, r.height, m_mdc.get(), r.x, r.y );
            blitted = true;
        }
    }
    if ( !blitted )
    {
        dc.Blit( clip.x, clip.y, clip.width, clip.height, m_mdc.get(), clip.x, clip.y );
    }

    m_mdc->SelectObject( wxNullBitmap );
}

void Panel::refreshDisplayValue( const Counter& counter )
{
    if ( !m_mdc || !m_numbersDC )
    {
        Refresh();
        return;
    }

    m_mdc->SelectObject( *m_bitmapPanelBuffer );

    counter.clear( m_bitmapControlBgBuffer, *m_mdc );
    counter.render( m_mdc, m_numbersDC );

    m_mdc->SelectObject( wxNullBitmap );

    RefreshRect( counter.bounds(), false );
}

bool Panel::addScore( uint32_t delta )
{
    if ( delta == 0u )
        return false;

    m_score.addValue( delta );
    refreshDisplayValue( m_score );

    bool justBrokePreviousRecord = false;
    if ( m_score.value() > m_hiScore.value() )
    {
        m_hiScore.reset( m_score.value() );
        m_hiScoreDirty = true;
        refreshDisplayValue( m_hiScore );

        if ( m_runStartHiScore > 0u && !m_newRecordNotified )
        {
            m_newRecordNotified = true;
            justBrokePreviousRecord = true;
        }
    }

    return justBrokePreviousRecord;
}

void Panel::increaseScore()
{
    addScore( 1u );
}

unsigned short Panel::currentLevel() const
{
    return static_cast<unsigned short>( m_level.value() );
}

unsigned short Panel::increaseLevel()
{
    m_level.increaseValue();
    saveHiScoreIfDirty();

    if ( IsShown() )
    {
        activate();
    }

    return static_cast<unsigned short>( m_level.value() );
}

unsigned char Panel::decreaseLives()
{
    m_lives.decreaseValue();
    saveHiScoreIfDirty();
    refreshDisplayValue( m_lives );

    return static_cast<unsigned char>( m_lives.value() );
}

void Panel::reset()
{
    saveHiScoreIfDirty();
    m_isLaunched = false;
    m_lives.reset( INITIAL_LIVES_VALUE );
    m_level.reset();
    m_score.reset();
    m_runStartHiScore = m_hiScore.value();
    m_newRecordNotified = false;
}
