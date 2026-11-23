cmake_minimum_required(VERSION 3.14.7 FATAL_ERROR)

# 1. Standalone analytical BallPhysics unit tests (no GUI/OpenGL required, runs on every build)
add_executable( ut_BallPhysics test_ball_physics.cpp )
target_compile_features( ut_BallPhysics PRIVATE cxx_std_17 )
target_include_directories( ut_BallPhysics PRIVATE "${CMAKE_SOURCE_DIR}/src/Shapes" )
add_test( NAME ut_BallPhysics COMMAND ut_BallPhysics )
add_custom_command(
    TARGET ut_BallPhysics POST_BUILD
    COMMAND $<TARGET_FILE:ut_BallPhysics>
    COMMENT "Running ut_BallPhysics unit tests"
)
add_dependencies( ${CMAKE_PROJECT_NAME} ut_BallPhysics )

# 2. Full GoogleTest/GoogleMock integration suite (ut_Fortress)
if( TARGET GTest::gtest AND TARGET GTest::gmock )
    include( GoogleTest )

    set( NAME ut_Fortress )

    file( GLOB_RECURSE APP_SOURCES "${CMAKE_SOURCE_DIR}/src/*.cpp" )
    list( FILTER APP_SOURCES EXCLUDE REGEX ".*/main\.cpp$" )

    add_executable(
        ${NAME}
        entry.cpp
        TestApp.hpp
        memory_leak.h
        ${APP_SOURCES}
    )

    target_compile_features( ${NAME} PRIVATE cxx_std_17 )

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
        target_link_libraries( ${NAME} PRIVATE libglew_shared dwmapi )
    elseif( UNIX )
        target_link_libraries( ${NAME} PRIVATE libGLEW${BUILD_POSTFIX}.so.2.2.0 )
    endif()

    add_dependencies( ${NAME} ${CMAKE_PROJECT_NAME} )
    gtest_discover_tests( ${NAME} DISCOVERY_MODE PRE_TEST )

    if( DEFINED ENV{CI} )
        # Headless CI runners only have software GDI OpenGL 1.1 (no GLSL shaders);
        # run the non-shader integration tests automatically during build.
        add_custom_command(
            TARGET ${NAME} POST_BUILD
            WORKING_DIRECTORY $<TARGET_FILE_DIR:${NAME}>
            COMMAND $<TARGET_FILE:${NAME}> --gtest_filter=TestFortress.App_Creation:TestFortress.Tools_Creation:TestFortress.Frame_Creation
            COMMENT "Running ut_Fortress headless integration tests"
        )
    else()
        add_custom_command(
            TARGET ${NAME} POST_BUILD
            WORKING_DIRECTORY $<TARGET_FILE_DIR:${NAME}>
            COMMAND $<TARGET_FILE:${NAME}>
            COMMENT "Running ut_Fortress unit tests"
        )
    endif()
endif()
