// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWidgets headers)
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include "Base.h"
#include "Brick.h"

using namespace Shapes;

// default brick size
const wxSize Brick::s_defaultSize = { 57, 27 };

Brick::Brick( int x, int y, const std::wstring& filename )
 : Base()
{
    if ( filename.empty() ) // empty place
    {
        m_size = s_defaultSize;
        m_alive = false;
    }
    else
        load( wxT( "/../resources/images/Bricks/" ) + filename );

    m_position = { x * m_size.GetWidth(), y * m_size.GetHeight() };
}

void Brick::draw( wxDC& dc )
{
    if ( m_painted )
        return;

    Base::draw( dc );
    m_painted = true;
}

void Brick::kill( wxDC& dc )
{
    clear( dc );
    m_alive = false;
}