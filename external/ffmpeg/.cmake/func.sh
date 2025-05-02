#!/bin/bash
#
# find_cmake_project_path
#   $1 - результат: путь к cmake проекту
#
function find_cmake_project_path() {
    if [ -z "$1" ]; then
        echo "${FUNCNAME}: Необходимо указать переменную для возвращаемого результата"
        exit 1
    fi

    local fcpp_prj_dir_
    fcpp_prj_dir_=$(pwd)
    while [ ! -e "${fcpp_prj_dir_}/CMakeLists.txt" ] && [ "${fcpp_prj_dir_}" != "/" ]; do
        fcpp_prj_dir_=$(dirname "${fcpp_prj_dir_}")
    done

    if [ "${fcpp_prj_dir_}" != "/" ]; then
        eval "$1"="\"$(realpath -s "${fcpp_prj_dir_}")\""
    else
        eval "$1"=
    fi 
}

#
# get_cmake_project_name
#   $1 - путь к cmake проекту
#   $2 - результат: название проекта
#
function get_cmake_project_name() {
    if [ "$#" -eq 1 ] || [ -z "$1" ]; then
        echo "${FUNCNAME}: Необходимо указать путь к cmake проекту"
        exit 1
    fi
    if [ -z "$2" ]; then
        echo "${FUNCNAME}: Необходимо указать переменную для возвращаемого результата"
        exit 1
    fi

    local prj_name
    prj_name="$(sed -Ez -e "s/.*\s*project\s*\(\s*(\w+).*/\1/g" "$1"/CMakeLists.txt)"

    if [ -n "${prj_name}" ]; then
        eval "$2"="${prj_name}"
    else
        eval "$2"=
    fi
}
