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
#include <wx/mstream.h>
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
    std::lock_guard<std::mutex> lock( m_zipMutex );

    // Release existing zip stream before its backing file stream to prevent UAF on reload
    m_zip.reset();
    m_zipFile.reset();
    m_cat.clear();

    // open the zip
    m_zipFile = std::make_shared<wxFFileInputStream>( getFullFileName( ( "/" + wxTheApp->GetAppName() + ".dat" ).ToStdString() ) );
    m_zip = std::make_shared<wxZipInputStream>( *m_zipFile );

    // load the zip catalog
    ZipEntryPtr entry;
    while ( entry.reset( m_zip->GetNextEntry() ), entry.get() != nullptr )
        m_cat[ entry->GetInternalName() ] = std::move( entry );
}

std::vector<uint8_t> Tools::loadResourceBytes( const std::string& name ) const
{
    std::lock_guard<std::mutex> lock( m_zipMutex );

    if ( !m_zip )
    {
        throw std::runtime_error( "Resource archive is not loaded" );
    }

    const auto it = m_cat.find( wxZipEntry::GetInternalName( name ) );
    if ( it == m_cat.end() )
    {
        throw std::out_of_range( "Can't find resource: " + name );
    }

    if ( !m_zip->OpenEntry( *it->second ) )
    {
        throw std::runtime_error( "Failed to open resource: " + name );
    }

    std::vector<uint8_t> data;
    const wxFileOffset entrySize = it->second->GetSize();
    if ( entrySize != wxInvalidOffset && entrySize > 0 )
    {
        data.reserve( static_cast<size_t>( entrySize ) );
    }

    uint8_t chunk[ 16384 ];
    while ( !m_zip->Eof() )
    {
        m_zip->Read( chunk, sizeof( chunk ) );
        const size_t n = m_zip->LastRead();
        if ( n == 0 )
            break;
        data.insert( data.end(), chunk, chunk + n );
    }

    const wxStreamError err = m_zip->GetLastError();
    if ( err != wxSTREAM_NO_ERROR && err != wxSTREAM_EOF )
    {
        throw std::runtime_error( "Failed to read resource: " + name );
    }

    return data;
}

std::shared_ptr<wxBitmap> Tools::loadBitmap( const std::string &name ) const
{
    const auto bytes = loadResourceBytes( name );
    wxMemoryInputStream memStream( bytes.data(), bytes.size() );
    auto bitmap = std::make_shared<wxBitmap>( wxImage { memStream, wxBITMAP_TYPE_PNG } );
    if ( !bitmap || !bitmap->IsOk() )
    {
        throw std::runtime_error( "Error loading resource: " + name );
    }

    return bitmap;
}

std::vector< std::vector<int> > Tools::loadLevel( const std::string &name, unsigned short levelNum ) const
{
    const auto bytes = loadResourceBytes( name );
    wxMemoryInputStream input( bytes.data(), bytes.size() );

    if ( !input.IsOk() )
    {
        throw std::runtime_error( "Error loading level map file" );
    }

    const wxString label = wxT( "Level " );
    const wxString finishLabel = wxT( "End" );

    wxTextInputStream text( input );
    wxString leveLabel( label );
    wxString line;

    leveLabel << levelNum;

    // Seek to level label
    while ( line != leveLabel && line != finishLabel && !input.Eof() )
    {
        line = text.ReadLine();
        line.Trim( true ).Trim( false );
    }

    // Check finishLabel BEFORE input.Eof() so "End" without a trailing newline is honored.
    if ( line == finishLabel )
    {
        throw stage_complete_exception {};
    }

    if ( line != leveLabel )
    {
        throw std::runtime_error( "Error loading level map file, file is corrupt or has wrong format" );
    }

    std::vector< std::vector<int> > bricks;
    wxStringTokenizer tkz;

    while ( !input.Eof() )
    {
        line = text.ReadLine();
        line.Trim( true ).Trim( false );
        if ( line.IsEmpty() || line.StartsWith( label ) || line == finishLabel )
            break;

        std::vector<int> inner;
        tkz.SetString( line );
        while ( tkz.HasMoreTokens() )
        {
            inner.emplace_back( wxAtoi( tkz.GetNextToken() ) );
        }

        if ( !inner.empty() )
            bricks.emplace_back( std::move( inner ) );
    }

    if ( bricks.empty() )
    {
        throw std::runtime_error( "Error loading level map file, file is corrupt or has wrong format" );
    }

    return bricks;
}

