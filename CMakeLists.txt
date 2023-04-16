cmake_minimum_required( VERSION 3.14.7 FATAL_ERROR )
set( CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake" )
set( NAME wxFortress )

set( CMAKE_GENERATOR_PLATFORM Win32 )
project( ${NAME} )

# Turn on the ability to create folders to organize projects (.vcproj)
# It creates "CMakePredefinedTargets" folder by default and adds CMake
# defined projects like INSTALL.vcproj and ZERO_CHECK.vcproj
set_property( GLOBAL PROPERTY USE_FOLDERS ON )

# Disable ZERO_CHECK target
set( CMAKE_SUPPRESS_REGENERATION true )

set( TODO ON CACHE BOOL "Enable compiletime TODO messages" )

set( RESOURCE_INSTALL_DIR "" CACHE PATH "Path to install resources to (leave empty for running uninstalled)" )

if( NOT CMAKE_ARCHIVE_OUTPUT_DIRECTORY )
  set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/.lib/static" )
endif()

if( NOT CMAKE_LIBRARY_OUTPUT_DIRECTORY )
  set( CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/.lib" )
endif()

if( NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY )
  set( CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/.bin" )
endif()

# set output directories for all builds (Debug, Release, etc.)
foreach( OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES} )
    string( TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG )
    set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_BINARY_DIR}/.lib/${OUTPUTCONFIG} )
    set( CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_BINARY_DIR}/.lib/${OUTPUTCONFIG} )
    set( CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_BINARY_DIR}/.bin/${OUTPUTCONFIG} )
endforeach( OUTPUTCONFIG CMAKE_CONFIGURATION_TYPES )

if( WIN32 )
    file( GLOB_RECURSE SOURCE_HEADERS "src/*.h" "src/*.hpp" )
    file( GLOB_RECURSE SOURCE_FILES "src/*.cpp" )

    source_group( TREE "src" FILES ${SOURCE_HEADERS} ${SOURCE_FILES} )

    add_executable(
       ${PROJECT_NAME} 
       WIN32 
       ${SOURCE_HEADERS}
       ${SOURCE_FILES}
    )
     
    add_subdirectory( external )

    set_property( DIRECTORY PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME} )
endif()

target_compile_features( ${PROJECT_NAME} PRIVATE cxx_std_17 )

target_include_directories(
	${PROJECT_NAME} 
	PRIVATE 
	${FFMPEG_INCLUDE_DIRS}
	${CMAKE_CURRENT_LIST_DIR}
	${CMAKE_CURRENT_LIST_DIR}/src
)

target_link_libraries(
	${PROJECT_NAME}
	glm::glm
	wx::core
	wx::base
	wx::gl
	OpenAL::OpenAL
	libglew_shared
	FFmpeg::FFmpeg
)

# it required for build with ffmpeg
target_link_options(${CMAKE_PROJECT_NAME} PUBLIC "$<$<C_COMPILER_ID:MSVC>:-SAFESEH:NO>")

# copy ffmpeg dll's
if( WIN32 )
    # list of required files
    set( ffmpeg_dlls_path ${PROJECT_SOURCE_DIR}/external/ffmpeg/win32/lib/dlls )
    set( ffmpeg_dlls
        ${ffmpeg_dlls_path}/avcodec-58.dll
        ${ffmpeg_dlls_path}/avformat-58.dll
        ${ffmpeg_dlls_path}/avutil-56.dll
        ${ffmpeg_dlls_path}/swresample-3.dll 
    )

    foreach( dll_file ${ffmpeg_dlls} )
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${dll_file}
                $<TARGET_FILE_DIR:${PROJECT_NAME}>
        )
   endforeach( dll_file ffmpeg_dlls ) 
endif()

# pack resources
file( GLOB_RECURSE ZIP_FILES 
    ${PROJECT_SOURCE_DIR}/resources/images/* 
    ${PROJECT_SOURCE_DIR}/resources/shaders/*
    ${PROJECT_SOURCE_DIR}/resources/sounds/*
    ${PROJECT_SOURCE_DIR}/levels.txt 
)

add_custom_command( TARGET ${PROJECT_NAME} POST_BUILD 
    COMMAND ${CMAKE_COMMAND} -E tar cvf ${CMAKE_BINARY_DIR}/.bin/$<CONFIG>/${PROJECT_NAME}.dat --format=zip ${ZIP_FILES}
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${PROJECT_SOURCE_DIR}/resources/video/Intro.mp4 
    ${CMAKE_BINARY_DIR}/.bin/$<CONFIG>/${PROJECT_NAME}.vid
)
