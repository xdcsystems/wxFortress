cmake_minimum_required( VERSION 3.14.7 FATAL_ERROR )
set( CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake" )
set( NAME wxArkanoid )

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
  set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/lib/static" )
endif()

if( NOT CMAKE_LIBRARY_OUTPUT_DIRECTORY )
  set( CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/lib" )
endif()

if( NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY )
  set( CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin" )
endif()

# set output directories for all builds (Debug, Release, etc.)
foreach( OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES} )
    string( TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG )
    set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_SOURCE_DIR}/lib/${OUTPUTCONFIG} )
    set( CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_SOURCE_DIR}/lib/${OUTPUTCONFIG} )
    set( CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_SOURCE_DIR}/bin/${OUTPUTCONFIG} )
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
# PRIVATE 
 ${FFMPEG_INCLUDE_DIRS}
 ${CMAKE_CURRENT_LIST_DIR}
 "${CMAKE_CURRENT_LIST_DIR}/src"
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

#set_target_properties(MyTarget PROPERTIES
#    VS_GLOBAL_RunCodeAnalysis false
#
#    # Use visual studio core guidelines
#    VS_GLOBAL_EnableMicrosoftCodeAnalysis false
#    #VS_GLOBAL_CodeAnalysisRuleSet ${CMAKE_CURRENT_SOURCE_DIR}/foo.ruleset
#    #VS_GLOBAL_CodeAnalysisRuleSet ${CMAKE_CURRENT_SOURCE_DIR}/foo.ruleset
#
#    # Use clangtidy
#    VS_GLOBAL_EnableClangTidyCodeAnalysis true
#    VS_GLOBAL_ClangTidyChecks "-* \"\"--config-file=${MY_CLANG_TIDY_CONFIG_PATH} --header-filter=.*" 
#)