#pragma once

#include <memory>

// Forward declarations
class wxImage;

// Texture2D is able to store and configure a texture in OpenGL.
// It also hosts utility functions for easy management.
class Texture2D
{
    public:
        // constructor (sets default texture modes)
        Texture2D();
        
        // generates texture from image data
        void generate( const wxImage &image );
        
        // binds the texture as the current active GL_TEXTURE_2D texture object
        void bind() const;

    public:
        // holds the ID of the texture object, used for all texture operations to reference to this particular texture
        unsigned int ID; // 0 is reserved, glGenBuffers() will return non-zero id if success

        // texture image dimensions
        int Width, Height; // width and height of loaded image in pixels

        // texture configuration
        unsigned int Wrap_S; // wrapping mode on S axis
        unsigned int Wrap_T; // wrapping mode on T axis
        unsigned int Filter_Min; // filtering mode if texture pixels < screen pixels
        unsigned int Filter_Max; // filtering mode if texture pixels > screen pixels
};

using texture2DPtr = std::shared_ptr<Texture2D>;
