#pragma once

#include <glm/glm.hpp>


// Forward declarations
class Texture2D;
class Shader;

enum class TextRendererState : unsigned char
{
    FINISHED,
    HELP,
};

class TextRenderer
{
    using texture2DPtr = std::shared_ptr<Texture2D>;
    using shaderPtr = std::shared_ptr<Shader>;

    public:
        TextRenderer( wxWindow* parent );
        ~TextRenderer();

        void renderFrame();
        void print( const std::string &text, int x, int y, const glm::vec2& size );
        void cleanup();

        void switchToFinishState( unsigned short stage );
        void switchToHelpState();

    private:
        void renderFinishMessage();
        void renderHelp();

        texture2DPtr m_texture;
        shaderPtr     m_shader;

        unsigned int m_vertexBufferID = 0;
        unsigned int m_UVBufferID = 0;
        unsigned int m_uniformID = 0;
        unsigned int m_attrVertex = 0;
        unsigned int m_attrUVs = 0;

        unsigned short m_col = 0;
        unsigned short m_row = 0;
        unsigned short m_delay = 57;
        
        unsigned short m_stage = 0;

        float m_startLinePosition = .0f;

        std::vector<std::string> m_message;
        std::vector<glm::vec2> m_vertices;
        std::vector<glm::vec2> m_UVs;

        std::shared_ptr<Timer> m_timer;

        wxEvtHandler* m_eventHandler { nullptr };
        wxCommandEvent m_eventCharShow;

        TextRendererState m_state = TextRendererState::FINISHED;
};

