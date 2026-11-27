cmake_minimum_required(VERSION 3.14.7 FATAL_ERROR)

set( WXFORTRESS_TEST_COMMANDS )
set( WXFORTRESS_TEST_TARGETS )

# 1. Analytical BallPhysics GoogleTest suite (no GUI/OpenGL required)
if( TARGET GTest::gtest_main )
    include( GoogleTest )
    add_executable( ut_BallPhysics test_ball_physics.cpp test_hardware_audio.cpp test_frame_pacer.cpp test_intro_skip_hold.cpp test_resonance.cpp
        test_brick_mechanics.cpp )
    target_compile_features( ut_BallPhysics PRIVATE cxx_std_17 )
    target_include_directories(
        ut_BallPhysics
        PRIVATE
        "${CMAKE_SOURCE_DIR}/src"
        "${CMAKE_SOURCE_DIR}/src/Common"
        "${CMAKE_SOURCE_DIR}/src/Shapes"
        "${CMAKE_SOURCE_DIR}/src/Video"
        "${CMAKE_SOURCE_DIR}/src/Sounds"
    )
    target_link_libraries( ut_BallPhysics PRIVATE GTest::gtest_main glm::glm )
    gtest_discover_tests( ut_BallPhysics DISCOVERY_MODE PRE_TEST )

    list( APPEND WXFORTRESS_TEST_TARGETS ut_BallPhysics )
    list( APPEND WXFORTRESS_TEST_COMMANDS COMMAND $<TARGET_FILE:ut_BallPhysics> )
endif()

# 2. Full GoogleTest/GoogleMock integration suite (ut_Fortress)
if( TARGET GTest::gtest AND TARGET GTest::gmock )
    include( GoogleTest )

    set( NAME ut_Fortress )

    file( GLOB_RECURSE APP_SOURCES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/*.cpp" )

    add_executable(
        ${NAME}
        entry.cpp
        TestApp.hpp
        memory_leak.h
        ${APP_SOURCES}
    )

    target_compile_features( ${NAME} PRIVATE cxx_std_17 )
    target_compile_definitions( ${NAME} PRIVATE WXFORTRESS_TESTING )

    if( MSVC )
        target_compile_definitions( ${NAME} PRIVATE $<$<CONFIG:Debug>:_CONSOLE> )
        set_target_properties( ${NAME} PROPERTIES COMPILE_FLAGS "/EHa" )
        set_target_properties( ${NAME} PROPERTIES LINK_FLAGS_DEBUG "/SUBSYSTEM:CONSOLE" )
        set_target_properties( ${NAME} PROPERTIES LINK_FLAGS_RELWITHDEBINFO "/SUBSYSTEM:CONSOLE" )
        target_link_options( ${NAME} PUBLIC -SAFESEH:NO )
    endif()

    target_include_directories(
        ${NAME}
        PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        "${CMAKE_SOURCE_DIR}"
        "${CMAKE_SOURCE_DIR}/src"
        "${CMAKE_BINARY_DIR}/generated"
        "${CMAKE_SOURCE_DIR}/external/glew-cmake/include"
        ${FFMPEG_INCLUDE_DIRS}
    )

    target_link_libraries(
        ${NAME}
        PRIVATE
        GTest::gtest
        GTest::gmock
        glm::glm
        wx::core
        wx::base
        wx::gl
        OpenAL::OpenAL
        FFmpeg::FFmpeg
    )

    if( WIN32 )
        target_link_libraries( ${NAME} PRIVATE libglew_shared dwmapi winmm )
    elseif( UNIX )
        if( TARGET glew )
            target_link_libraries( ${NAME} PRIVATE glew )
        elseif( TARGET libglew_shared )
            target_link_libraries( ${NAME} PRIVATE libglew_shared )
        else()
            target_link_libraries( ${NAME} PRIVATE GLEW )
        endif()
        target_link_libraries( ${NAME} PRIVATE ${WXFORTRESS_LINUX_EXTRA_LIBS} )
    endif()

    add_dependencies( ${NAME} ${CMAKE_PROJECT_NAME} )
    gtest_discover_tests( ${NAME} DISCOVERY_MODE PRE_TEST )

    list( APPEND WXFORTRESS_TEST_TARGETS ${NAME} )
    if( DEFINED ENV{CI} )
        # Headless CI runners only have software GDI OpenGL 1.1 (no GLSL shaders);
        # run the non-shader integration tests automatically during build.
        list( APPEND WXFORTRESS_TEST_COMMANDS
            COMMAND $<TARGET_FILE:${NAME}> --gtest_filter=TestFortress.App_Creation:TestFortress.Tools_*:TestFortress.Frame_Creation )
    else()
        list( APPEND WXFORTRESS_TEST_COMMANDS COMMAND $<TARGET_FILE:${NAME}> )
    endif()
endif()

# Run ALL GoogleTest suites automatically on every default build
if( WXFORTRESS_TEST_TARGETS )
    add_custom_target(
        run_tests ALL
        ${WXFORTRESS_TEST_COMMANDS}
        DEPENDS ${WXFORTRESS_TEST_TARGETS}
        WORKING_DIRECTORY $<TARGET_FILE_DIR:${CMAKE_PROJECT_NAME}>
        COMMENT "Running ALL GoogleTest suites (ut_BallPhysics + ut_Fortress)"
        USES_TERMINAL
    )
endif()
