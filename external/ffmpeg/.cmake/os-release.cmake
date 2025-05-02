#[=============================================================================[

get_os_release(NAME VERSION)
-------------------------------

Получает название и версию дистрибутива Linux.

Параметры:
    NAME 
        название переменной, куда будет записано название дистрибутива

    VERSION 
        название переменной, куда будет записан номер версии дистрибутива



remove_version_patch(RES_VERSION SRC_VERSION)
---------------------------------------------

Удаляет номер патча из номера версии

Параметры:
    RESULT_VARIABLE
        название переменной, куда будет записан результат

    SRC_VERSION
        значение номера версии        


#]=============================================================================]

function(get_os_release 
    name 
    version
    )
    execute_process(
        COMMAND sh -c "cat /etc/os-release 2>/dev/null | grep -x -E 'ID=.+' | sed s/ID=//"
        OUTPUT_VARIABLE os_name
        RESULT_VARIABLE err_code
        ERROR_QUIET
        )
    if (NOT err_code EQUAL 0)
        set(${name} "" PARENT_SCOPE)
        return()
    endif()
    string(STRIP "${os_name}" os_name)
    string(REGEX REPLACE "\"(.+)\"" "\\1" os_name "${os_name}")
    set(${name} "${os_name}" PARENT_SCOPE)

    execute_process(
        COMMAND sh -c "cat /etc/os-release 2>/dev/null | grep -x -E VERSION_ID=.+ | sed s/VERSION_ID=//"
        OUTPUT_VARIABLE os_version
        RESULT_VARIABLE err_code
        ERROR_QUIET
        )
    if (NOT err_code EQUAL 0)
        set(${version} "" PARENT_SCOPE)
        return()
    endif()
    string(STRIP "${os_version}" os_version)
    string(REGEX REPLACE "\"(.+)\"" "\\1" os_version "${os_version}")
    set(${version} "${os_version}" PARENT_SCOPE)
endfunction()

function(remove_version_patch 
    res_version
    src_version
    )
    string(REGEX REPLACE "(.+\\..+)\\..+" "\\1" version ${src_version})
    set(${res_version} "${version}" PARENT_SCOPE)
endfunction()

