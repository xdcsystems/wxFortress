#pragma once

#include <map>

#include <GL/glew.h>
#include "Singleton.hpp"

#include <wx/stdstream.h>
#include <wx/wfstream.h>
#include <wx/zipstrm.h>

// Forward declarations
class wxBitmap;
class wxPoint;
class wxSize;

void CheckOpenGLError( const char *stmt, const char *fname, int line );

#if defined( _DEBUG ) && !defined( SKIP_GL_CHECKS )
    #define GL_CHECK( stmt )                               \
    do                                                 \
    {                                                  \
        stmt;                                          \
        CheckOpenGLError( #stmt, __FILE__, __LINE__ ); \
    }                                                  \
    while ( 0 )
#else
    #define GL_CHECK( stmt ) stmt
#endif

struct stage_complete_exception : std::exception
{};

class Tools final : public Singleton<Tools>
{
    using ZipEntryPtr = std::unique_ptr<wxZipEntry>;
    using ZipCatalog = std::map<wxString, ZipEntryPtr>;
    using ZipStream = std::shared_ptr<wxZipInputStream>;
    using ZipFilePtr = std::shared_ptr<wxFFileInputStream>;

    public:
        explicit Tools( typename Singleton<Tools>::token ) {};

        std::string getFullFileName( const std::string &filename ) const;
        void loadResources();
        wxInputStream& loadResource( const std::string& name ) const;
        wxStdInputStream loadResourceStd( const std::string& name ) const;
        std::shared_ptr<wxBitmap> loadBitmap( const std::string &name ) const;
        std::vector< std::vector<int> > loadLevel( const std::string &name, unsigned short levelNum ) const;

        void bhmLine( std::vector<wxPoint> &trajectory, int x1, int y1, int x2, int y2, const wxSize &limits ) const;

    private:
        ZipEntryPtr m_entry;
        ZipCatalog m_cat;
        ZipFilePtr m_zipFile;
        ZipStream m_zip;
};
