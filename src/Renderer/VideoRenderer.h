#pragma once

// Forward declarations
class Shader;
class Texture2D;
class SpriteRenderer;

class VideoRenderer
{
        using shaderPtr = std::shared_ptr<Shader>;
        using texture2DPtr = std::shared_ptr<Texture2D>;

    public:
        VideoRenderer();
        ~VideoRenderer();

        bool ok();
        void setViewport( int x, int y, int width, int height );
        
        void draw( int width, int height, uint8_t **data );
        void drawNV12( int width, int height, uint8_t** data );

        int viewWidth() const { return m_viewWidth; };
        int viewHeight() const { return m_viewHeight; };

    private:
        void init();
        bool bindBuffers( int width, int height );

        unsigned int m_VBO { 0 };
        unsigned int m_VAO { 0 };
        unsigned int m_EBO { 0 };
        unsigned int m_texs[ 3 ] {};

        unsigned int m_attrVertex{ 0 };
        unsigned int m_attrUVs{ 0 };

        float m_ratio { 1 };
        int m_viewWidth { -1 };
        int m_viewHeight { -1 };
        int m_texWidth { -1 };
        int m_texHeight { -1 };

        shaderPtr m_shader;
        shaderPtr m_shaderNV12;
};
