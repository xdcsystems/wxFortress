
function(find_gtest_packages VERSION)
    find_package(GTest ${VERSION}) 
    find_package(GoogleMock ${VERSION})

    if(NOT GTest_FOUND OR NOT GoogleMock_FOUND)
        unset(BUILD_TESTING CACHE)
    endif()
endfunction()

#
#	set_repository_gtest_dependency
#
function(set_repository_gtest_dependency)
    if(NOT BUILD_TESTING)
        return()
    endif()

    set(options)
    set(one_value_args VERSION)
    set(multi_value_args)
    cmake_parse_arguments(ARG
        "${options}" "${one_value_args}" "${multi_value_args}"
        ${ARGN})

    if(NOT ARG_VERSION)
        set(ARG_VERSION 1.8)
    endif()
    set(cmake_find_package_calls 
        "find_gtest_packages(${ARG_VERSION})" 
        )

    set_repository_library_dependency(
        NAME GTest
        VERSION ${ARG_VERSION}
        DESCRIPTION "GoogleTest - библиотека Unit-тестирования"
        PACKAGE "libgtest-dev libgmock-dev?"
        CMAKE_FIND_PACKAGE_CALLS ${cmake_find_package_calls}
        )
endfunction()

