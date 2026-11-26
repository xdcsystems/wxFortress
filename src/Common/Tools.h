#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <vector>

#include <GL/glew.h>
#include "Common/defs.h"
#include "Singleton.hpp"

#include <wx/wfstream.h>
#include <wx/zipstrm.h>

// Forward declarations
class wxBitmap;

void CheckOpenGLError( const char *stmt, const char *fname, int line );

#if SKIP_GL_CHECKS == 0
    #define GL_CHECK( stmt )                           \
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
        std::vector<uint8_t> loadResourceBytes( const std::string& name ) const;
        std::shared_ptr<wxBitmap> loadBitmap( const std::string &name ) const;
        std::vector< std::vector<int> > loadLevel( const std::string &name, unsigned short levelNum ) const;


    private:
        mutable std::mutex m_zipMutex;
        ZipCatalog m_cat;
        ZipFilePtr m_zipFile;
        ZipStream m_zip;
};
