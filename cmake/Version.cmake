# Automatic version derivation from Git tags (supports both v1.2.3 and legacy v.1.0.1 tags).
# Produces:
#   WXFORTRESS_VERSION_MAJOR, MINOR, PATCH, TWEAK (commit count since tag)
#   WXFORTRESS_GIT_HASH
#   WXFORTRESS_VERSION_STRING  (e.g. "1.1.0" on exact tag, "1.0.1.17 (6da980f)" between tags)
#   WXFORTRESS_VERSION_DEBIAN  (e.g. "1.1.0" on exact tag, "1.0.1+git17.6da980f" between tags)

set( WXFORTRESS_VERSION_MAJOR 1 )
set( WXFORTRESS_VERSION_MINOR 0 )
set( WXFORTRESS_VERSION_PATCH 1 )
set( WXFORTRESS_VERSION_TWEAK 0 )
set( WXFORTRESS_GIT_HASH "unknown" )

find_package( Git QUIET )
if( GIT_FOUND AND EXISTS "${CMAKE_SOURCE_DIR}/.git" )
    execute_process(
        COMMAND ${GIT_EXECUTABLE} describe --tags --long --match "v*" --abbrev=7
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        OUTPUT_VARIABLE _git_desc
        ERROR_QUIET
        OUTPUT_STRIP_TRAILING_WHITESPACE
        RESULT_VARIABLE _git_desc_rc
    )
    if( _git_desc_rc EQUAL 0 AND _git_desc MATCHES "^v\\.?([0-9]+)\\.([0-9]+)\\.([0-9]+)-([0-9]+)-g([0-9a-fA-F]+)$" )
        set( WXFORTRESS_VERSION_MAJOR "${CMAKE_MATCH_1}" )
        set( WXFORTRESS_VERSION_MINOR "${CMAKE_MATCH_2}" )
        set( WXFORTRESS_VERSION_PATCH "${CMAKE_MATCH_3}" )
        set( WXFORTRESS_VERSION_TWEAK "${CMAKE_MATCH_4}" )
        set( WXFORTRESS_GIT_HASH      "${CMAKE_MATCH_5}" )
    else()
        execute_process(
            COMMAND ${GIT_EXECUTABLE} rev-parse --short=7 HEAD
            WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
            OUTPUT_VARIABLE _git_sha
            ERROR_QUIET
            OUTPUT_STRIP_TRAILING_WHITESPACE
            RESULT_VARIABLE _git_sha_rc
        )
        if( _git_sha_rc EQUAL 0 AND _git_sha )
            set( WXFORTRESS_GIT_HASH "${_git_sha}" )
        endif()
    endif()
endif()

set( WXFORTRESS_VERSION_NUMERIC
    "${WXFORTRESS_VERSION_MAJOR}.${WXFORTRESS_VERSION_MINOR}.${WXFORTRESS_VERSION_PATCH}.${WXFORTRESS_VERSION_TWEAK}"
)

if( WXFORTRESS_VERSION_TWEAK EQUAL 0 )
    set( WXFORTRESS_VERSION_STRING "${WXFORTRESS_VERSION_MAJOR}.${WXFORTRESS_VERSION_MINOR}.${WXFORTRESS_VERSION_PATCH}" )
    set( WXFORTRESS_VERSION_DEBIAN "${WXFORTRESS_VERSION_MAJOR}.${WXFORTRESS_VERSION_MINOR}.${WXFORTRESS_VERSION_PATCH}" )
else()
    set( WXFORTRESS_VERSION_STRING "${WXFORTRESS_VERSION_NUMERIC} (${WXFORTRESS_GIT_HASH})" )
    set( WXFORTRESS_VERSION_DEBIAN "${WXFORTRESS_VERSION_MAJOR}.${WXFORTRESS_VERSION_MINOR}.${WXFORTRESS_VERSION_PATCH}+git${WXFORTRESS_VERSION_TWEAK}.${WXFORTRESS_GIT_HASH}" )
endif()

message( STATUS "wxFortress version: ${WXFORTRESS_VERSION_STRING} (Debian: ${WXFORTRESS_VERSION_DEBIAN})" )
