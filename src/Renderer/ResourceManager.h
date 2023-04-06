#pragma once

#include <map>
#include <string>


// Forward declarations
class Texture2D;
class Shader;

class ResourceManager
{
    using texture2DPtr = std::shared_ptr<Texture2D>;
    using shaderPtr = std::shared_ptr<Shader>;

    public:
        // resource storage
        inline static std::map<std::string, shaderPtr> s_shaders;
        inline static std::map<std::string, texture2DPtr> s_textures;

        // loads (and generates) a shader program from file loading vertex, fragment (and geometry) shader's source code. If gShaderFile is not nullptr, it also loads a geometry shader
        static shaderPtr LoadShader( const std::string& vShaderFile, const std::string& fShaderFile, const std::string& gShaderFile, const std::string& name );

        // retrieves a stored sader
        static shaderPtr GetShader( const std::string& name );

        // loads (and generates) a texture from file
        static texture2DPtr LoadTexture( const std::string& fileName, const std::string& textureName );

        // retrieves a stored texture
        static texture2DPtr GetTexture( const std::string& name );

        // properly de-allocates all loaded resources
        static void Clear();

    private:
        // private constructor, that is we do not want any actual resource manager objects. Its members and functions should be publicly available (static).
        ResourceManager() = default;

        // loads and generates a shader from file
        static shaderPtr LoadShaderFromFile( const std::string& vShaderFile, const std::string& fShaderFile, const std::string& gShaderFile = "" );

        // loads a single texture from file
        static texture2DPtr LoadTextureFromFile( const std::string& fileName );
};
