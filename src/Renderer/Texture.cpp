// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWidgets headers)
#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include <GL/glew.h>

#include "Common/Tools.h"
#include "Texture.h"


Texture2D::Texture2D()
    : ID( 0 )
    , Width( -1 )
    , Height( -1 )
    , Internal_Format( GL_RGB )
    , Image_Format( GL_RGB )
    , Wrap_S( GL_CLAMP_TO_EDGE )
    , Wrap_T( GL_CLAMP_TO_EDGE )
    , Filter_Min( GL_LINEAR )
    , Filter_Max( GL_LINEAR )
{
    GL_CHECK( glGenTextures( 1, &ID ) );
}

void Texture2D::generate( wxImage* image )
{
    Width = image->GetWidth();
    Height = image->GetHeight();

    // create Texture
    GL_CHECK( glActiveTexture( GL_TEXTURE0  ) );
    GL_CHECK( glBindTexture( GL_TEXTURE_2D, ID ) );

    // https://www.khronos.org/opengl/wiki/Common_Mistakes#The_Object_Oriented_Language_Problem
    //glPixelStorei( GL_UNPACK_ALIGNMENT, 4 );

    const int bytesPerPixel = image->HasAlpha() ? 4 : 3;
    GLubyte* bitmapData = image->GetData();
    GLubyte* alphaData = image->GetAlpha();
    GLubyte* imageData = nullptr;

    // note: must make a local copy before passing the data to OpenGL, as GetData() returns RGB 
    // and we want the Alpha channel if it's present. Additionally OpenGL seems to interpret the 
    // data upside-down so we need to compensate for that.
    int imageSize = Width * Height * bytesPerPixel;
    imageData = new GLubyte[ imageSize ];

    int revValue = Height - 1;

    for ( int y = 0; y < Height; ++y )
    {
        for ( int x = 0; x < Width; ++x )
        {
            imageData[ ( x + y * Width ) * bytesPerPixel + 0 ] =
                bitmapData[ ( x + ( revValue - y ) * Width ) * 3 ];

            imageData[ ( x + y * Width ) * bytesPerPixel + 1 ] =
                bitmapData[ ( x + ( revValue - y ) * Width ) * 3 + 1 ];

            imageData[ ( x + y * Width ) * bytesPerPixel + 2 ] =
                bitmapData[ ( x + ( revValue - y ) * Width ) * 3 + 2 ];

            if ( bytesPerPixel == 4 )
            {
                imageData[ ( x + y * Width ) * bytesPerPixel + 3 ] =
                    alphaData[ x + ( revValue - y ) * Width ];
            }
        }
    }

    //// if only one texture
    //GL_CHECK( glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0 ) );
    //GL_CHECK( glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0 ) );

    //// set Texture wrap and filter modes
    //GL_CHECK( glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, Filter_Min ) );
    //GL_CHECK( glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, Filter_Max ) );
    //GL_CHECK( glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, Wrap_S ) );
    //GL_CHECK( glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, Wrap_T ) );

    // auto generate MIPMAP
    GL_CHECK( glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT ) );
    GL_CHECK( glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT ) );
    GL_CHECK( glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR ) );
    GL_CHECK( glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR ) );
    GL_CHECK( glTexParameteri( GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE ) );

    // if yes, everything is fine
    GL_CHECK( glTexImage2D( GL_TEXTURE_2D,
        0,
        image->HasAlpha() ? GL_RGBA8 : GL_RGB8, // bytesPerPixel
        Width,
        Height,
        0,
        image->HasAlpha() ? GL_RGBA : GL_RGB,
        GL_UNSIGNED_BYTE,
        imageData ) );

    delete[] imageData;

    // unbind texture
    GL_CHECK( glBindTexture( GL_TEXTURE_2D, 0 ) );
}

void Texture2D::bind() const
{
    GL_CHECK( glBindTexture( GL_TEXTURE_2D, ID ) );
}