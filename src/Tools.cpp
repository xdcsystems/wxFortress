// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWidgets headers)
#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include <wx/stdpaths.h>
#include <wx/filename.h>
#include <wx/wfstream.h>
#include <wx/txtstrm.h>
#include <wx/tokenzr.h>

#include "Tools.h"


wxString Tools::getFullFileName( const std::wstring& filename ) const
{
    wxFileName fileName( wxStandardPaths::Get().GetDataDir() + filename );
    fileName.Normalize( wxPATH_NORM_ABSOLUTE | wxPATH_NORM_DOTS );

    auto fullFileName = fileName.GetFullPath();
    if ( fullFileName.empty() || !wxFileExists( fullFileName ) )
    {
        throw std::runtime_error( "Error loading resource: " + filename );
    }

    return fullFileName;
}

std::shared_ptr<wxBitmap> Tools::loadBitmapFromFile( const std::wstring& filename ) const
{
    const auto &fullFileName = getFullFileName( filename );
    auto bitmap = std::make_shared<wxBitmap>( fullFileName, wxBITMAP_TYPE_PNG );
    if ( !bitmap || !bitmap->IsOk() )
    {
        throw std::runtime_error( "Error loading resource: " + filename );
    }

    return bitmap;
}

std::vector< std::vector<int>> Tools::loadLevelFromFile( const std::wstring &filename, unsigned short levelNum ) const
{
    wxFileInputStream input( getFullFileName( wxT( "/../resources/levels.txt" ) ) );
    
    if ( !input.IsOk() )
        throw std::runtime_error( "Error loading level map file" );

    wxTextInputStream text( input );
    wxString line;
    wxString leveLabel;

    leveLabel << wxT( "Level " ) << levelNum;

    // Seek to level label
    while ( line != leveLabel && !input.Eof() )
        line = text.ReadLine();

    if ( input.Eof() )
        throw std::runtime_error( "Error loading level map file, file is corrupt or has wrong format" );

    std::vector< std::vector<int>> bricks;
    wxStringTokenizer tkz;

    while ( !line.IsEmpty() && !input.Eof() )
    {
        std::vector<int> inner;

        line = text.ReadLine();
        if ( line.IsEmpty() || line.Contains( wxT( "Level " ) ) )
            break;

        tkz.SetString( line );
        while ( tkz.HasMoreTokens() )
            inner.push_back( wxAtoi( tkz.GetNextToken() ) );

        bricks.push_back( std::move( inner ) );
    }

    if ( bricks.empty() )
        throw std::runtime_error( "Error loading level map file, file is corrupt or has wrong format" );

    return bricks;
}

// Bresenham Line Drawing Algorithm
void Tools::bhmLine( std::vector<wxPoint> &trajectory, int x1, int y1, int x2, int y2, const wxSize& limits ) const
{
    trajectory.clear();

    int dx = x2 - x1;
    // if x1 == x2, then it does not matter what we set here
    const int ix( ( dx > 0 ) - ( dx < 0 ) );

    dx = abs( dx ) << 1;

    int dy = y2 - y1;
    // if y1 == y2, then it does not matter what we set here
    const int iy( ( dy > 0 ) - ( dy < 0 ) );
    dy = abs( dy ) << 1;

    // retVal.push_back( wxPoint( x1, y1 ) );

    if ( dx >= dy )
    {
        // error may go below zero
        int error( dy - ( dx >> 1 ) );

        while ( x1 != x2 && x1 >= -1 && y1 >= -1 && x1 <= limits.x + 1 && y1 <= limits.y )
        {
            if ( ( error >= 0 ) && ( error || ( ix > 0 ) ) )
            {
                error -= dx;
                y1 += iy;
            }
            // else do nothing

            error += dy;
            x1 += ix;

            if ( x1 >= -1 && y1 >= -1 && x1 <= limits.x + 1 && y1 <= limits.y )
                trajectory.push_back( { x1, y1 } );
        }
    }
    else
    {
        // error may go below zero
        int error( dx - ( dy >> 1 ) );

        while ( y1 != y2 && x1 >= -1 && y1 >= -1 && x1 <= limits.x + 1 && y1 <= limits.y )
        {
            if ( ( error >= 0 ) && ( error || ( iy > 0 ) ) )
            {
                error -= dy;
                x1 += ix;
            }
            // else do nothing

            error += dx;
            y1 += iy;

            if ( x1 >= -1 && y1 >= -1 && x1 <= limits.x + 1 && y1 <= limits.y )
                trajectory.push_back( { x1, y1 } );
        }
    }
}