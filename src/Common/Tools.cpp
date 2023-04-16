// For compilers that support precompilation, includes "wx/wx.h".
#include <wx/wxprec.h>

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWidgets headers)
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include <map>

#include <wx/stdpaths.h>
#include <wx/filename.h>
#include <wx/wfstream.h>
#include <wx/txtstrm.h>
#include <wx/tokenzr.h>

#include "Common/defs.h"
#include "Tools.h"


void CheckOpenGLError( const char *stmt, const char *fname, int line )
{
    GLenum errorCode( 0 );
    while ( ( errorCode = glGetError() ) != GL_NO_ERROR )
    {
        std::string error;
        switch ( errorCode )
        {
            case GL_INVALID_ENUM: error = "INVALID_ENUM"; break;
            case GL_INVALID_VALUE: error = "INVALID_VALUE"; break;
            case GL_OUT_OF_MEMORY: error = "OUT_OF_MEMORY"; break;
            case GL_STACK_OVERFLOW: error = "STACK_OVERFLOW"; break;
            case GL_STACK_UNDERFLOW: error = "STACK_UNDERFLOW"; break;
            case GL_INVALID_OPERATION : error = "INVALID_OPERATION"; break;
            case GL_INVALID_FRAMEBUFFER_OPERATION: error = "INVALID_FRAMEBUFFER_OPERATION"; break;
            default: break;
        }
        wxLogMessage( "[OpenGL ERROR : %s in file %s line ( %d ) - for %s", error, fname, line, stmt );
    }
}

std::string Tools::getFullFileName( const std::string &filename ) const
{
    const auto path = wxFileName( wxStandardPaths::Get().GetExecutablePath() ).GetPath();
    wxFileName fileName( path + filename );
    fileName.Normalize( wxPATH_NORM_ABSOLUTE | wxPATH_NORM_DOTS );

    auto fullFileName = fileName.GetFullPath();
    if ( fullFileName.empty() || !wxFileExists( fullFileName ) )
    {
        throw std::runtime_error( "Error loading resource: " + filename );
    }

    return fullFileName.ToStdString();
}

void Tools::loadResources()
{
    // open the zip
    m_zipFile = std::make_shared<wxFFileInputStream>( getFullFileName( ( "/" + wxTheApp->GetAppName() + ".dat" ).ToStdString() ) );
    m_zip = std::make_shared<wxZipInputStream>( *m_zipFile );

    // load the zip catalog
    while ( m_entry.reset( m_zip->GetNextEntry() ), m_entry.get() != nullptr )
        m_cat[ m_entry->GetInternalName() ] = std::move( m_entry );
}

wxInputStream& Tools::loadResource( const std::string& name ) const
{
    // open an entry by name
    if ( auto it = m_cat.find( wxZipEntry::GetInternalName( name ) ); it != m_cat.end() )
    {
        m_zip->OpenEntry( *it->second );
        return *m_zip.get();
    }

    throw std::out_of_range( "Can't find resource: " + name );
}

wxStdInputStream Tools::loadResourceStd( const std::string& name ) const
{
    return { loadResource( name ) };
}

std::shared_ptr<wxBitmap> Tools::loadBitmap( const std::string &name ) const
{
    auto bitmap = std::make_shared<wxBitmap>( wxImage { loadResource( name ), wxBITMAP_TYPE_PNG } );
    if ( !bitmap || !bitmap->IsOk() )
    {
        throw std::runtime_error( "Error loading resource: " + name );
    }

    return bitmap;
}

std::vector< std::vector<int> > Tools::loadLevel( const std::string &name, unsigned short levelNum ) const
{
    wxInputStream &input( loadResource( name ) );

    if ( !input.IsOk() )
    {
        throw std::runtime_error( "Error loading level map file" );
    }

    const wxString label = wxT( "Level " );
    const wxString finishLabel = wxT( "End" );

    wxTextInputStream text( loadResource( name ) );
    wxString leveLabel( label );
    wxString line;

    leveLabel << levelNum;

    // Seek to level label
    while ( line != leveLabel && !line.Contains( finishLabel ) && !input.Eof() )
    {
        line = text.ReadLine();
    }

    if ( input.Eof() )
    {
        throw std::runtime_error( "Error loading level map file, file is corrupt or has wrong format" );
    }

    if ( line.Contains( finishLabel ) )
    {
        throw stage_complete_exception {};
    }

    std::vector< std::vector<int> > bricks;
    wxStringTokenizer tkz;

    while ( !line.IsEmpty() && !input.Eof() )
    {
        std::vector<int> inner;

        line = text.ReadLine();
        if ( line.IsEmpty() || line.Contains( label ) || line.Contains( finishLabel ) )
            break;

        tkz.SetString( line );
        while ( tkz.HasMoreTokens() )
        {
            inner.emplace_back( wxAtoi( tkz.GetNextToken() ) );
        }

        bricks.emplace_back( std::move( inner ) );
    }

    if ( bricks.empty() )
    {
        throw std::runtime_error( "Error loading level map file, file is corrupt or has wrong format" );
    }

    return bricks;
}

// Bresenham Line Drawing Algorithm
void Tools::bhmLine( std::vector<wxPoint> &trajectory, int x1, int y1, int x2, int y2, const wxSize &limits ) const
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
            {
                trajectory.emplace_back( x1, y1 );
            }
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
            {
                trajectory.emplace_back( x1, y1 );
            }
        }
    }
}