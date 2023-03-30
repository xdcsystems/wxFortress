#pragma once

#include <GL/glew.h>
#include "Singleton.hpp"

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
    public:
        explicit Tools( typename Singleton<Tools>::token ) {};

        std::string getFullFileName( const std::string &filename ) const;
        std::shared_ptr<wxBitmap> loadBitmapFromFile( const std::string &filename ) const;
        std::vector< std::vector<int> > loadLevelFromFile( const std::string &filename, unsigned short levelNum ) const;

        void bhmLine( std::vector<wxPoint> &trajectory, int x1, int y1, int x2, int y2, const wxSize &limits ) const;
};
