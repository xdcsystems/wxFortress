#pragma once

#include "Renderer/ResourceManager.h"

class TestApp : public App
{
    public:
        MOCK_METHOD( bool, OnInit, ( ), ( override ) );
        MOCK_METHOD( int, OnRun, ( ), ( override ) );

        void clear()
        {
            if ( m_canvas && m_context )
                m_canvas->SetCurrent( *m_context );

            // Release GL-owning objects while the OpenGL context is still alive.
            m_ball.reset();
            m_brick.reset();
            m_spriteRenderer.reset();
            if ( m_canvas && m_context )
                ResourceManager::Clear();

            m_context.reset();
            if ( m_canvas )
            {
                m_canvas->Destroy();
                m_canvas = nullptr;
            }
            if ( m_mainFrame )
            {
                m_mainFrame->Destroy();
                m_mainFrame = nullptr;
            }
        }

        void createFrame()
        {
            if ( m_mainFrame )
                return;

            m_mainFrame = new wxFrame();
            m_mainFrame->Create(
                nullptr,
                wxID_ANY,
                wxTheApp->GetAppName(),
                wxDefaultPosition,
                { 1024, 768 },
                ( wxDEFAULT_FRAME_STYLE | wxTAB_TRAVERSAL | wxCLIP_CHILDREN ) & ~( wxRESIZE_BORDER | wxMAXIMIZE_BOX ),
                wxASCII_STR( wxFrameNameStr ) );

            wxInitAllImageHandlers();
        }

        void createCanvas()
        {
            if ( m_canvas )
                return;

            m_canvas = new wxGLCanvas( m_mainFrame );
            m_context = std::make_shared<wxGLContext>( m_canvas );
        }

        void initGLEW()
        {
            createFrame();
            createCanvas();

            m_canvas->SetCurrent( *m_context );

            glewExperimental = true;
            GLenum err = glewInit(); // Check error
        }

        void initOpenGL()
        {
            GL_CHECK( glClearColor( 0.0, 0.0, 0.0, 1.0 ) );
            GL_CHECK( glEnable( GL_TEXTURE_2D ) );
            GL_CHECK( glEnable( GL_COLOR_MATERIAL ) );
            GL_CHECK( glEnable( GL_BLEND ) );
            GL_CHECK( glDisable( GL_DEPTH_TEST ) );
            GL_CHECK( glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA ) );
        }

        void createRenderer()
        {
            m_spriteRenderer = std::make_shared<SpriteRenderer>();
        }

        void createBrick()
        {
            m_brick = std::make_shared<Shapes::Brick>(
                glm::vec2 { 100.f, 100.f },
                Shapes::BrickType::RED );
        }

        void createBall()
        {
            m_ball = std::make_shared<Shapes::Ball>();
        }

    public:
        wxFrame* m_mainFrame{ nullptr };
        wxGLCanvas* m_canvas{ nullptr };
        std::shared_ptr<wxGLContext> m_context;
        std::shared_ptr<SpriteRenderer> m_spriteRenderer;
        std::shared_ptr<Shapes::Brick> m_brick;
        std::shared_ptr < Shapes::Ball> m_ball;
};

class TestFortress : public ::testing::Test
{
    protected:
        void SetUp()
        {
            m_testApp = new TestApp();
            wxApp::SetInstance( m_testApp );
            wxEntryStart( m_argc, m_argv );
        }

        void TearDown()
        {
            if ( m_testApp )
                m_testApp->clear();
            wxTheApp->OnExit();
            wxEntryCleanup();
        }

        TestApp* m_testApp;
        char* m_argv[ 5 ] = { const_cast<char*>( "wxFortress" ), const_cast<char*>( "arg1" ), const_cast<char*>( "arg2 with spaces" ), const_cast<char*>( "arg3" ), nullptr };
        int m_argc = sizeof( m_argv ) / sizeof( m_argv[ 0 ] ) - 1;
};