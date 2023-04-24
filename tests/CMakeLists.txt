cmake_minimum_required(VERSION 3.14.7 FATAL_ERROR)

set( NAME ut_Fortress )

project( ${NAME} )

include(GoogleTest)

if( WIN32 )
    file( GLOB_RECURSE SOURCE_HEADERS "*.h" "*.hpp" )
    file( GLOB_RECURSE SOURCE_FILES "*.cpp" )

    set( OBJS_PATH ${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}.dir/$<CONFIG> )
    set( PROJECT_OBJS
	${OBJS_PATH}/Audio.obj
	${OBJS_PATH}/Ball.obj
	${OBJS_PATH}/Base.obj
	${OBJS_PATH}/Board.obj
	${OBJS_PATH}/Brick.obj
	${OBJS_PATH}/Bricks.obj
	${OBJS_PATH}/CountDown.obj
	${OBJS_PATH}/Explosion.obj
	${OBJS_PATH}/Explosions.obj
	${OBJS_PATH}/main.obj
	${OBJS_PATH}/MainFrame.obj
	${OBJS_PATH}/MediaManager.obj
	${OBJS_PATH}/MoveController.obj
	${OBJS_PATH}/Movie.obj
	${OBJS_PATH}/Overlay.obj
	${OBJS_PATH}/Panel.obj
	${OBJS_PATH}/ParticleGenerator.obj
	${OBJS_PATH}/RenderWindow.obj
	${OBJS_PATH}/ResourceManager.obj
	${OBJS_PATH}/Shader.obj
	${OBJS_PATH}/ShapesManager.obj
	${OBJS_PATH}/SoundManager.obj 
	${OBJS_PATH}/SpriteRenderer.obj
	${OBJS_PATH}/TextRenderer.obj
	${OBJS_PATH}/Texture.obj
	${OBJS_PATH}/Timer.obj
	${OBJS_PATH}/Tools.obj
	${OBJS_PATH}/Video.obj
	${OBJS_PATH}/VideoRenderer.obj
    )

    set_source_files_properties(
        ${PROJECT_OBJS}
        PROPERTIES
        EXTERNAL_OBJECT true
        GENERATED true
    )

    set( CMAKE_EXE_LINKER_FLAGS 
         "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup"
    )

    add_executable(
       ${PROJECT_NAME} 
       WIN32 
       ${SOURCE_HEADERS}
       ${SOURCE_FILES}
    )

endif()

if (MSVC)
	target_compile_definitions(${PROJECT_NAME} PRIVATE $<$<CONFIG:Debug>:_CONSOLE>)
        set_target_properties( ${PROJECT_NAME} PROPERTIES COMPILE_FLAGS "/EHa")
        set_target_properties( ${PROJECT_NAME} PROPERTIES LINK_FLAGS_DEBUG "/SUBSYSTEM:CONSOLE")
        set_target_properties( ${PROJECT_NAME} PROPERTIES COMPILE_DEFINITIONS_DEBUG "_CONSOLE")
        set_target_properties( ${PROJECT_NAME} PROPERTIES LINK_FLAGS_RELWITHDEBINFO "/SUBSYSTEM:CONSOLE")
        set_target_properties( ${PROJECT_NAME} PROPERTIES COMPILE_DEFINITIONS_RELWITHDEBINFO "_CONSOLE")
endif()

target_compile_features( ${PROJECT_NAME} PRIVATE cxx_std_17 )

if( WIN32 )
    # for dwmapi.lib
    target_link_directories(${CMAKE_PROJECT_NAME} PUBLIC "${PROJECT_SOURCE_DIR}/external/directX/x86/")
    target_link_libraries(
	${PROJECT_NAME}
	dwmapi.lib
    )
endif()

# it required for build with ffmpeg
target_link_options( ${PROJECT_NAME} PUBLIC "$<$<C_COMPILER_ID:MSVC>:-SAFESEH:NO>")

set( PROJECT_DIR "${CMAKE_SOURCE_DIR}" )
set( PROJECT_DIR_SRC "${CMAKE_SOURCE_DIR}/src" )
set( GTEST_ROOT "${CMAKE_SOURCE_DIR}/external/googletest/" )

#message(STATUS " CMAKE_PROJECT_NAME: ${CMAKE_PROJECT_NAME}")
#message(STATUS " PROJECT_DIR: ${PROJECT_DIR}")
#message(STATUS " PROJECT_DIR_SRC: ${PROJECT_DIR_SRC}")
#message(STATUS " GTEST_ROOT: ${GTEST_ROOT}")
#message(STATUS " CMAKE_CURRENT_LIST_DIR: ${CMAKE_CURRENT_LIST_DIR}")

target_include_directories(
    ${PROJECT_NAME} 
    PRIVATE 
    ${CMAKE_CURRENT_LIST_DIR}
    ${PROJECT_DIR}
    ${PROJECT_DIR_SRC}
    ${FFMPEG_INCLUDE_DIRS}
)

target_link_libraries(
    ${PROJECT_NAME}
    GTest::gtest
    GTest::gtest_main
    GTest::gmock
    GTest::gmock_main
    glm::glm
    wx::core
    wx::base
    wx::gl
    OpenAL::OpenAL
    libglew_shared
    FFmpeg::FFmpeg
    ${PROJECT_OBJS}
)

add_dependencies( ${PROJECT_NAME} ${CMAKE_PROJECT_NAME} )

gtest_discover_tests( ${PROJECT_NAME} )

add_custom_command(TARGET ${PROJECT_NAME}
                   COMMENT "Run tests"
                   WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
                   #COMMAND ${CMAKE_CTEST_COMMAND} -C $<CONFIGURATION> -R "^${PROJECT_NAME}$" --verbose
                   COMMAND ${PROJECT_NAME} --verbose
)
