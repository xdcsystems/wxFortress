
class TestApp : public App
{
    public:
        MOCK_METHOD( bool, OnInit, ( ), ( override ) );
        MOCK_METHOD( int, OnRun, ( ), ( override ) );

        void clear()
        {
            if ( m_canvas )
                m_canvas.reset();
        }

        void createFrame()
        {
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
            m_canvas = std::make_shared<wxGLCanvas>( m_mainFrame );
            m_context = std::make_shared<wxGLContext>( m_canvas.get() );
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
                Shapes::BrickType::RED,
                Shapes::Brick::SizeOf( Shapes::BrickType::RED ) );
        }

        void createBall()
        {
            m_ball = std::make_shared<Shapes::Ball>();
        }

    public:
        wxFrame* m_mainFrame{ nullptr };
        std::shared_ptr<wxGLCanvas> m_canvas;
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
            wxTheApp->OnExit();
            wxEntryCleanup();
        }

        TestApp* m_testApp;
        char* m_argv[ 5 ] = { "wxFortress", "arg1", "arg2 with spaces", "arg3", nullptr };
        int m_argc = sizeof( m_argv ) / sizeof( m_argv[ 0 ] ) - 1;
};