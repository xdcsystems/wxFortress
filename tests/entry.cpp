#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "memory_leak.h"

// For compilers that support precompilation, includes "wx/wx.h".
#include <wx/wxprec.h>

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWidgets headers)
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include <thread>
#include <vector>

#include <GL/glew.h>
#include <wx/glcanvas.h>

#include "Common/Tools.h"
#include "App.h"

#include "Renderer/SpriteRenderer.h"
#include "Shapes/Base.h"
#include "Shapes/Brick.h"
#include "Shapes/Ball.h"

#include "TestApp.hpp"


using ::testing::Return;

TEST_F( TestFortress, App_Creation )
{
    EXPECT_EQ( m_argv[ 0 ], wxTheApp->GetAppName() );
    EXPECT_CALL( *m_testApp, OnInit() ).WillOnce( Return( true ) );
    EXPECT_CALL( *m_testApp, OnRun() ).WillOnce( Return( 0 ) );

    wxTheApp->CallOnInit();
    wxTheApp->OnRun();
}

TEST_F( TestFortress, Tools_Creation )
{
    EXPECT_NO_THROW( Tools::Instance().loadResources() );
}

TEST_F( TestFortress, Tools_LoadResourceBytes_ThreadSafe )
{
    ASSERT_NO_THROW( Tools::Instance().loadResources() );

    const auto baseline = Tools::Instance().loadResourceBytes( "resources/levels.txt" );
    ASSERT_FALSE( baseline.empty() );

    constexpr int kThreads = 4;
    std::vector<std::vector<uint8_t>> results( kThreads );
    std::vector<std::thread> workers;
    workers.reserve( kThreads );

    for ( int i = 0; i < kThreads; ++i )
    {
        workers.emplace_back( [&, i]()
        {
            results[ i ] = Tools::Instance().loadResourceBytes( "resources/levels.txt" );
        } );
    }
    for ( auto& t : workers )
        t.join();

    for ( int i = 0; i < kThreads; ++i )
        EXPECT_EQ( results[ i ], baseline );

    EXPECT_THROW( Tools::Instance().loadResourceBytes( "resources/does_not_exist.bin" ), std::out_of_range );
}

TEST_F( TestFortress, Tools_LoadLevel_AllLevelsAndStageComplete )
{
    ASSERT_NO_THROW( Tools::Instance().loadResources() );

    for ( unsigned short lvl = 1; lvl <= 5; ++lvl )
    {
        const auto grid = Tools::Instance().loadLevel( "resources/levels.txt", lvl );
        EXPECT_FALSE( grid.empty() ) << "Level " << lvl << " should not be empty";
    }

    EXPECT_THROW( Tools::Instance().loadLevel( "resources/levels.txt", 6 ), stage_complete_exception );
}

TEST_F( TestFortress, Frame_Creation )
{
    EXPECT_NO_THROW( m_testApp->createFrame() );
}

TEST_F( TestFortress, Canvas_Creation )
{
    EXPECT_NO_THROW( m_testApp->createFrame() );
    EXPECT_NO_THROW( m_testApp->createCanvas() );

    EXPECT_NO_THROW( m_testApp->clear() );
}

TEST_F( TestFortress, GLEW_Init )
{
    EXPECT_NO_THROW( m_testApp->createFrame() );
    EXPECT_NO_THROW( m_testApp->createCanvas() );

    EXPECT_NO_THROW( m_testApp->initGLEW() );

    EXPECT_NO_THROW( m_testApp->clear() );
}

TEST_F( TestFortress, OpenGL_Init )
{
    EXPECT_NO_THROW( m_testApp->createFrame() );
    EXPECT_NO_THROW( m_testApp->createCanvas() );

    EXPECT_NO_THROW( m_testApp->initGLEW() );
    EXPECT_NO_THROW( m_testApp->initOpenGL() );

    EXPECT_NO_THROW( m_testApp->clear() );
}

TEST_F( TestFortress, SpriteRenderer_Creation )
{
    EXPECT_NO_THROW( m_testApp->createFrame() );
    EXPECT_NO_THROW( m_testApp->createCanvas() );

    EXPECT_NO_THROW( m_testApp->initGLEW() );
    EXPECT_NO_THROW( m_testApp->initOpenGL() );

    EXPECT_NO_THROW( m_testApp->createRenderer() );
    
    EXPECT_NO_THROW( m_testApp->clear() );
}

TEST_F( TestFortress, Brick_Creation )
{
    EXPECT_NO_THROW( m_testApp->createFrame() );
    EXPECT_NO_THROW( m_testApp->createCanvas() );

    EXPECT_NO_THROW( m_testApp->initGLEW() );
    EXPECT_NO_THROW( m_testApp->initOpenGL() );

    EXPECT_NO_THROW( m_testApp->createRenderer() );
    EXPECT_NO_THROW( m_testApp->createBrick() );

    EXPECT_NO_THROW( m_testApp->clear() );
}

TEST_F( TestFortress, Ball_Creation )
{
    EXPECT_NO_THROW( m_testApp->createFrame() );
    EXPECT_NO_THROW( m_testApp->createCanvas() );

    EXPECT_NO_THROW( m_testApp->initGLEW() );
    EXPECT_NO_THROW( m_testApp->initOpenGL() );

    EXPECT_NO_THROW( m_testApp->createRenderer() );
    EXPECT_NO_THROW( m_testApp->createBall() );

    EXPECT_NO_THROW( m_testApp->clear() );
}

// Useful launch options --gtest_filter=<test_case_name.test_name|test_case_name.*> --gtest_repeat=1 --gtest_catch_exceptions=0
int main( int argc, char *argv[] )
{
  ::setlocale(LC_ALL, "Russian");
  testing::InitGoogleTest(&argc, argv);

  return RUN_ALL_TESTS();
}
