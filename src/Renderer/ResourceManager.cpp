// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWidgets headers)
#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include <GL/glew.h>

#include <iostream>
#include <sstream>
#include <fstream>

#include "Common/Tools.h"
#include "Texture.h"
#include "Shader.h"
#include "ResourceManager.h"

// Instantiate static variables
std::map<std::string, texture2DPtr> ResourceManager::s_textures;
std::map<std::string, shaderPtr> ResourceManager::s_shaders;


shaderPtr ResourceManager::LoadShader( const std::string& vShaderFile, const std::string& fShaderFile, const std::string& gShaderFile, const std::string& name )
{
    s_shaders[ name ] = LoadShaderFromFile( vShaderFile, fShaderFile, gShaderFile );
    return s_shaders[ name ];
}

shaderPtr ResourceManager::GetShader( const std::string& name )
{
    return s_shaders[ name ];
}

texture2DPtr ResourceManager::LoadTexture( const std::string& fileName, bool alpha, const std::string& textureName )
{
    s_textures[ textureName ] = LoadTextureFromFile( fileName, alpha );
    return s_textures[ textureName ];
}

texture2DPtr ResourceManager::GetTexture( const std::string& name )
{
    return s_textures[ name ];
}

void ResourceManager::Clear()
{
    // (properly) delete all shaders	
    for ( auto iter : s_shaders )
        glDeleteProgram( iter.second->ID );
    
    // (properly) delete all textures
    for ( auto iter : s_textures )
        glDeleteTextures( 1, &iter.second->ID );
}

shaderPtr ResourceManager::LoadShaderFromFile( const std::string& vShaderFile, const std::string& fShaderFile, const std::string& gShaderFile )
{
    // 1. retrieve the vertex/fragment source code from filePath
    std::string vertexCode;
    std::string fragmentCode;
    std::string geometryCode;
    try
    {
        // open files
        std::ifstream vertexShaderFile( Tools::Instance().getFullFileName( vShaderFile ) );
        std::ifstream fragmentShaderFile( Tools::Instance().getFullFileName( fShaderFile ) );
        std::stringstream vShaderStream, fShaderStream;
        
        // read file's buffer contents into streams
        vShaderStream << vertexShaderFile.rdbuf();
        fShaderStream << fragmentShaderFile.rdbuf();
        
        // close file handlers
        vertexShaderFile.close();
        fragmentShaderFile.close();
        
        // convert stream into string
        vertexCode = vShaderStream.str();
        fragmentCode = fShaderStream.str();
        
        // if geometry shader path is present, also load a geometry shader
        if ( !gShaderFile.empty() )
        {
            std::ifstream geometryShaderFile( Tools::Instance().getFullFileName( gShaderFile ) );
            std::stringstream gShaderStream;
            gShaderStream << geometryShaderFile.rdbuf();
            geometryShaderFile.close();
            geometryCode = gShaderStream.str();
        }
    }
    catch ( std::exception e )
    {
        wxString what;
        what.Printf( "ERROR::SHADER: Failed to read shader files, \"%s\"", e.what() );
        wxLogMessage( what );
    }
    const char* vShaderCode = vertexCode.c_str();
    const char* fShaderCode = fragmentCode.c_str();
    const char* gShaderCode = geometryCode.c_str();
    
    // 2. now create shader object from source code
    auto shader = std::make_shared<Shader>();
    shader->compile( vShaderCode, fShaderCode, gShaderFile.empty() ? nullptr : gShaderCode );

    return shader;
}

texture2DPtr ResourceManager::LoadTextureFromFile( const std::string& fileName, bool alpha )
{
    // create texture object
    auto texture = std::make_shared<Texture2D>();
    if ( alpha )
    {
        texture->Internal_Format = GL_RGBA;
        texture->Image_Format = GL_RGBA;
    }

    auto image = std::make_shared<wxImage>( Tools::Instance().getFullFileName( fileName ) );
    texture->generate( image.get() );

    return texture;
}