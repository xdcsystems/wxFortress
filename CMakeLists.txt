cmake_minimum_required( VERSION 3.14.7 FATAL_ERROR )

set( CMAKE_MODULE_PATH
    "${CMAKE_MODULE_PATH}"
    "${CMAKE_SOURCE_DIR}/cmake"
)
set( NAME wxFortress )

if( CMAKE_GENERATOR MATCHES "^Visual Studio" )
    set( CMAKE_GENERATOR_PLATFORM Win32 )
endif()

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose Release or Debug" FORCE)
endif()

get_property( _wxfortress_is_multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG )
if( _wxfortress_is_multi_config AND NOT CMAKE_CONFIGURATION_TYPES )
    set( CMAKE_CONFIGURATION_TYPES Release Debug )
endif()

# to distinguish between debug and release lib
if (CMAKE_BUILD_TYPE STREQUAL Debug)
    set(BUILD_POSTFIX "d")
else(CMAKE_BUILD_TYPE STREQUAL Debug)
    set(BUILD_POSTFIX "")
endif(CMAKE_BUILD_TYPE STREQUAL Debug)

include( "${CMAKE_SOURCE_DIR}/cmake/Version.cmake" )

project( ${NAME}
    VERSION ${WXFORTRESS_VERSION_MAJOR}.${WXFORTRESS_VERSION_MINOR}.${WXFORTRESS_VERSION_PATCH}.${WXFORTRESS_VERSION_TWEAK}
    DESCRIPTION "Simple Arkanoid game"
    LANGUAGES C CXX
)

configure_file(
    "${CMAKE_SOURCE_DIR}/src/Common/Version.h.in"
    "${CMAKE_BINARY_DIR}/generated/Common/Version.h"
    @ONLY
)
file( WRITE "${CMAKE_BINARY_DIR}/wxFortress_version.txt"
    "WXFORTRESS_VERSION_STRING=\"${WXFORTRESS_VERSION_STRING}\"\n"
    "WXFORTRESS_VERSION_DEBIAN=\"${WXFORTRESS_VERSION_DEBIAN}\"\n"
    "WXFORTRESS_VERSION_NUMERIC=\"${WXFORTRESS_VERSION_NUMERIC}\"\n"
)

list( APPEND CMAKE_MODULE_PATH
    "${PROJECT_SOURCE_DIR}/external/.cmake/"
)

include( ExternalLibraries )

if( WIN32 )
    # Turn on the ability to create folders to organize projects (.vcproj)
    # It creates "CMakePredefinedTargets" folder by default and adds CMake
    # defined projects like INSTALL.vcproj and ZERO_CHECK.vcproj
    set_property( GLOBAL PROPERTY USE_FOLDERS ON )
endif()

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

file( GLOB_RECURSE SOURCE_HEADERS CONFIGURE_DEPENDS "src/*.h" "src/*.hpp" )
file( GLOB_RECURSE SOURCE_FILES CONFIGURE_DEPENDS "src/*.cpp" )

if( CMAKE_CXX_COMPILER_ID STREQUAL "GNU" )
    # Must precede add_executable() so wxFortress, ut_Fortress, and vendored submodules all inherit it on GCC 13+.
    add_compile_options( "$<$<COMPILE_LANGUAGE:CXX>:SHELL:-include cstdint>" )
endif()

if( WIN32 )
    enable_language( RC )
    configure_file(
        "${CMAKE_SOURCE_DIR}/resources/wxFortress.rc.in"
        "${CMAKE_BINARY_DIR}/generated/wxFortress.rc"
        @ONLY
    )
    source_group( TREE "${CMAKE_CURRENT_SOURCE_DIR}/src" FILES ${SOURCE_HEADERS} ${SOURCE_FILES} )

    add_executable(
       ${PROJECT_NAME}
       WIN32
       ${SOURCE_HEADERS}
       ${SOURCE_FILES}
       "${CMAKE_BINARY_DIR}/generated/wxFortress.rc"
    )

    set_property( DIRECTORY PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME} )

elseif( UNIX )
    add_executable(
       ${PROJECT_NAME}
       ${SOURCE_HEADERS}
       ${SOURCE_FILES}
    )
endif()

target_compile_features( ${PROJECT_NAME} PRIVATE cxx_std_17 )

if( MSVC )
    target_compile_options( ${PROJECT_NAME} PRIVATE /W4 )
elseif( CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang" )
    target_compile_options( ${PROJECT_NAME} PRIVATE -Wall -Wextra )
endif()

# add external libs
add_subdirectory( external )

# Enforce pure scalar GLM codegen and zero-initialized vector constructors for GLM consumers only
if( TARGET glm )
    target_compile_definitions( glm INTERFACE GLM_FORCE_PURE GLM_FORCE_CTOR_INIT )
elseif( TARGET glm::glm )
    get_target_property( _glm_aliased glm::glm ALIASED_TARGET )
    if( _glm_aliased )
        target_compile_definitions( ${_glm_aliased} INTERFACE GLM_FORCE_PURE GLM_FORCE_CTOR_INIT )
    else()
        target_compile_definitions( glm::glm INTERFACE GLM_FORCE_PURE GLM_FORCE_CTOR_INIT )
    endif()
endif()

if( MINGW AND TARGET libglew_shared )
    target_link_libraries( libglew_shared PRIVATE msvcrt )
    target_link_options( libglew_shared PRIVATE -Wl,--allow-multiple-definition )
endif()

if( UNIX )
    set( WXFORTRESS_LINUX_EXTRA_LIBS dl pthread m )
    foreach( _fflib va va-drm va-x11 vdpau drm z bz2 lzma X11 )
        find_library( _fflib_path_${_fflib} NAMES ${_fflib} )
        if( _fflib_path_${_fflib} )
            list( APPEND WXFORTRESS_LINUX_EXTRA_LIBS ${_fflib_path_${_fflib}} )
        endif()
    endforeach()
    if( TARGET FFmpeg )
        # Attach static FFmpeg's transitive system libs directly to the FFmpeg target
        # so CMake's link-order solver always places them AFTER libavformat.a / libavcodec.a.
        target_link_libraries( FFmpeg INTERFACE ${WXFORTRESS_LINUX_EXTRA_LIBS} )
    endif()
endif()

# add tests
add_subdirectory_with_folder("tests" tests )

# GLEW
set(GLEW_INCLUDE_DIRS "${PROJECT_SOURCE_DIR}/external/glew-cmake/include")

target_include_directories(
    ${PROJECT_NAME}
    PRIVATE
    ${GLEW_INCLUDE_DIRS}
    ${FFMPEG_INCLUDE_DIRS}
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/src
    "${CMAKE_BINARY_DIR}/generated"
)

target_link_libraries(
    ${PROJECT_NAME}
    glm::glm
    wx::core
    wx::base
    wx::gl
    OpenAL::OpenAL
    FFmpeg::FFmpeg
)
if( UNIX )
    set_target_properties( ${PROJECT_NAME} PROPERTIES
        BUILD_RPATH "$ORIGIN"
        INSTALL_RPATH "$ORIGIN"
    )
    if( TARGET glew )
        target_link_libraries( ${PROJECT_NAME} glew )
    elseif( TARGET libglew_shared )
        target_link_libraries( ${PROJECT_NAME} libglew_shared )
    else()
        target_link_libraries( ${PROJECT_NAME} GLEW )
    endif()
    target_link_libraries( ${PROJECT_NAME} ${WXFORTRESS_LINUX_EXTRA_LIBS} )
endif()

if( WIN32 )
    # for dwmapi.lib
    target_link_directories(${CMAKE_PROJECT_NAME} PUBLIC "${PROJECT_SOURCE_DIR}/external/directX/x86/")
    target_link_libraries(
        ${PROJECT_NAME}
        libglew_shared
        dwmapi
        winmm
    )
endif()

if( MSVC )
    # it required for build with ffmpeg
    target_link_options(${CMAKE_PROJECT_NAME} PUBLIC -SAFESEH:NO)
    SET (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SAFESEH:NO")
    SET (CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} /SAFESEH:NO")
endif()

if( WIN32 )
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:libglew_shared>
            $<TARGET_FILE:OpenAL::OpenAL>
            $<TARGET_FILE:wx::base>
            $<TARGET_FILE:wx::core>
            $<TARGET_FILE:wx::gl>
            $<TARGET_FILE_DIR:${PROJECT_NAME}>
    )
    
    # copy ffmpeg dll's
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
file( GLOB_RECURSE ZIP_FILES CONFIGURE_DEPENDS
    ${PROJECT_SOURCE_DIR}/resources/images/*
    ${PROJECT_SOURCE_DIR}/resources/shaders/*
    ${PROJECT_SOURCE_DIR}/resources/sounds/*
    ${PROJECT_SOURCE_DIR}/levels.txt
)

string( TOUPPER ${CMAKE_BUILD_TYPE} OUTPUTCONFIG )

if( CMAKE_CONFIGURATION_TYPES )
    add_custom_command( TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E tar cvf $<TARGET_FILE_DIR:${PROJECT_NAME}>/${PROJECT_NAME}.dat --format=zip ${ZIP_FILES}
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    )
else()
    set( RESOURCE_DAT_FILE "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${PROJECT_NAME}.dat" )
    add_custom_command(
        OUTPUT "${RESOURCE_DAT_FILE}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
        COMMAND ${CMAKE_COMMAND} -E tar cvf "${RESOURCE_DAT_FILE}" --format=zip ${ZIP_FILES}
        DEPENDS ${ZIP_FILES}
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        COMMENT "Packing ${PROJECT_NAME}.dat resources"
    )
    add_custom_target( pack_resources ALL DEPENDS "${RESOURCE_DAT_FILE}" )
    add_dependencies( ${PROJECT_NAME} pack_resources )
endif()

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${PROJECT_SOURCE_DIR}/resources/video/Intro.mp4
    $<TARGET_FILE_DIR:${PROJECT_NAME}>/${PROJECT_NAME}.vid
)
