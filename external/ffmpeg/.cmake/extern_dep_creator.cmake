cmake_minimum_required(VERSION 3.7)

list(APPEND CMAKE_MODULE_PATH 
    ${CMAKE_CURRENT_LIST_DIR}
    )

include(extern_dep)
include(gsl)
include(gtest)

if(NOT DEFINED BUILD_TESTING)
    set(BUILD_TESTING ON)
endif()

create_project_dependencies(
    CONFIG_FILE ${CONFIG_FILE}
    DESTINATION_DIR ${DESTINATION_DIR}
    )

