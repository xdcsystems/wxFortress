#[=============================================================================[

Добавляет поддержку утилиты clang-tidy в сборку проекта

Выполняется поиск утилиты clang-tidy. В случае успеха устанавливается
значение переменной CLANG_TIDY в найденный путь и активизируется поддержка clang-tidy.
В противном случае значение CLANG_TIDY устанавливается в CLANG_TIDY-NOTFOUND и
поддержка clang-tidy становится недоступной.

Типы проверок для clang-tidy устанавливаются в переменной CLANG_TIDY_CHECKS_OPTION_VALUE.
Она имеет тот же формат как и соответствующий параметр -checks в утилите clang-tidy.



add_for_code_static_analize(TARGET <name> [SOURCES <source> ...])
-----------------------------------------------------------------

Добавляет файлы cmake-цели (созданной вызовом add_executable, add_library) в список файлов 
для проверки clang-tidy

Параметры:
    TARGET <name>
        Название cmake-цели
    SOURCES <source> ...
        Список файлов для проверки для стандартной (STATIC/SHARED) библиотеки;
        Для интерфейсной (INTERFACE)  библиотеки, вместо файлов нужно указывать 
        директории с заголовочными файлами. Вложенность директорий не используется.
        Если не задано, все файлы цели включаются в проверку



exclude_from_code_static_analize(TARGET <name> [SOURCES <source> ...])
----------------------------------------------------------------------

Удаляет файлы cmake-цели (созданной вызовом add_executable, add_library) из список файлов 
для проверки clang-tidy

Параметры:
    TARGET <name>
        Название цели
    SOURCES <source> ...
        Список файлов для исключения.
        Если не задано, все файлы цели исключаются из проверки



add_clang_tidy_check_target()
-------------------------------

Добавляет цели в makefile файлы для запуска clang-tidy по списку файлов, зарегистрированных 
функцией add_for_code_static_analize: 
1) цель clang-tidy-check в makefile корневого проекта для ВСЕХ зарегистрированных файлов
2) цель clang-tidy-check-<имя cmake-цели> в makefile для зарегистрированных файлов соответствующей cmake-цели

#]=============================================================================]


cmake_minimum_required(VERSION 3.7)

include(helpers)
root_include_guard()

function(_check_clang_tidy_version 
        out_version)
    unset(${out_version} PARENT_SCOPE)

    if(NOT CLANG_TIDY)
        return()
    endif()

    execute_process(
        COMMAND ${CLANG_TIDY} --version
	RESULT_VARIABLE exec_res
        OUTPUT_VARIABLE clang_tidy_vers_out
    )
    if (NOT exec_res EQUAL 0)
        message(WARNING "Error of execution '${CLANG_TIDY}'.")
 	return()   
    endif()

    string(REGEX MATCH "[ ]*LLVM version[ ]+([0-9]+(\.[0-9]+)*)[ ]*"
        clang_tidy_vers_line
        "${clang_tidy_vers_out}")
    set(clang_tidy_version ${CMAKE_MATCH_1})

    if(clang_tidy_version)
        if(clang_tidy_version VERSION_LESS "8")
            message(WARNING "Clang-tidy version is ${clang_tidy_version}. "
                "Versions are less 8.0 may bring incorrect issues in source code analyzing.")
        endif()

    	set(${out_version} ${clang_tidy_version} PARENT_SCOPE)

    else()
        message(WARNING "Unknown clang-tidy version.")
    endif()
endfunction()

function(_get_clang_tidy_checks_option_value 
        out_value
)
    set(checks_option "*") # init. all checks enabled
    set(checks
        -cppcoreguidelines-macro-usage.AllowedRegexp
        -cppcoreguidelines-pro-type-vararg
        -modernize-use-trailing-return-type
        -readability-braces-around-statements
        -readability-named-parameter
        -readability-redundant-access-specifiers
        -readability-identifier-length
        -modernize-use-nodiscard
        -google-default-arguments
        -google-readability-braces-around-statements
        -google-runtime-references
        -google-readability-todo
        -llvm-include-order
        -llvm-header-guard
        -llvmlibc-*
        -fuchsia*
        -hicpp*
        -cert-*
        -android*
        -altera*
    )

    foreach(check ${checks})
        string(APPEND checks_option ",${check}")
    endforeach()

    set(${out_value} ${checks_option} PARENT_SCOPE)
endfunction()

#
# Global clang-tidy setup
#

if (NOT CLANG_TIDY)
    unset(CLANG_TIDY CACHE)

    find_program(CLANG_TIDY clang-tidy)
endif()

if (CLANG_TIDY)
    _check_clang_tidy_version(CLANG_TIDY_VERSION)
    if (CLANG_TIDY_VERSION)
    	message(STATUS "clang-tidy utility path: '${CLANG_TIDY}', version: ${CLANG_TIDY_VERSION}")
        
        set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

        _get_clang_tidy_checks_option_value(CLANG_TIDY_CHECKS_OPTION_VALUE)

        set(CLANG_TIDY_HEADER_FILTER '.+\.h\(pp\)?')

        # Lists of source files, that will be analyzed
        set(_STATIC_ANALIZER_ALL_DEST_FILES "" CACHE INTERNAL "")
        set(_STATIC_ANALIZER_ALL_DEST_DIRS "" CACHE INTERNAL "")

        set(ct_templ_make_file 
            "${CMAKE_CURRENT_LIST_DIR}/clang-tidy-check.make.in")
    endif()
else()
    message(WARNING "clang-tidy utility not found")
endif()

function(_get_target_source_paths
        target_name
        out_target_src_path_list)
    unset(src_paths)

    get_target_property(target_type ${target_name} TYPE)
    if(target_type STREQUAL INTERFACE_LIBRARY)
        message(WARNING "Target '${target_name}' is an interface libary")
    else()
        get_target_property(target_srcs ${target_name} SOURCES)
        if(NOT target_srcs)
            message(WARNING "Target '${target_name}' doesn't contain sources")
        else()
            foreach(src ${target_srcs})
                get_source_file_property(path ${src} LOCATION)
                if(path)
                    list(APPEND src_paths ${path})
                endif()
            endforeach()
        endif()
    endif()
    set(${out_target_src_path_list} ${src_paths} PARENT_SCOPE)
endfunction()

#
# add_for_code_static_analize
#
function(add_for_code_static_analize)
    if(NOT CLANG_TIDY)
        return()
    endif()

    set(options)
    set(one_value_args TARGET)
    set(multi_value_args SOURCES)
    cmake_parse_arguments(ARG
        "${options}" "${one_value_args}" "${multi_value_args}"
        ${ARGN})
    if(ARG_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "Function 'add_for_code_static_analize()' called "
            "with unrecognized parameters: '${ARG_UNPARSED_ARGUMENTS}'")
    endif()

    if(NOT ARG_TARGET)
        message(FATAL_ERROR "The target argument must be defined")
    endif()
    if(NOT (TARGET ${ARG_TARGET}))
        message(FATAL_ERROR "'${ARG_TARGET}' is not a target")
    endif()

    set(has_dest_files FALSE)

    get_target_property(target_type ${ARG_TARGET} TYPE)
    if (NOT (target_type STREQUAL INTERFACE_LIBRARY))
        _get_target_source_paths(${ARG_TARGET} src_path_list)
        if (NOT src_path_list)
            return()
        endif()

        if(NOT ARG_SOURCES)
            set(file_dests ${src_path_list})
        else()
            set(file_dests)
            foreach(src ${ARG_SOURCES})
                get_source_file_property(path ${src} LOCATION)
                if(NOT path)
                    message(WARNING "The invalid file '${src}' "
                        "and will be ignored for the static analize.")
                    continue()
                endif()

                list(FIND src_path_list ${path} idx)
                if(idx EQUAL -1)
                    message(WARNING "The source '${src}' doesn't belong the target "
                        "'${ARG_TARGET}' and will be ignored for the static analize.")
                else()
                    list(APPEND file_dests ${path})
                endif()
            endforeach()
        endif()

        if(file_dests)
            _import_include_directories_dependences(${ARG_TARGET} file_dests)

            get_directory_property(target_file_dests STATIC_ANALIZER_${ARG_TARGET}_FILES)
            list(APPEND target_file_dests ${file_dests})
            list(REMOVE_DUPLICATES target_file_dests)
            set_directory_properties(PROPERTIES 
                    STATIC_ANALIZER_${ARG_TARGET}_FILES "${target_file_dests}")

            set(glob_dests ${_STATIC_ANALIZER_ALL_DEST_FILES} ${file_dests})
            list(REMOVE_DUPLICATES glob_dests)
            set(_STATIC_ANALIZER_ALL_DEST_FILES ${glob_dests} CACHE INTERNAL "")

            set(has_dest_files TRUE)
        endif()
    else()
        set(dir_dests)
        foreach(dir ${ARG_SOURCES})
            get_filename_component(dir ${dir} ABSOLUTE)
            if(NOT IS_DIRECTORY ${dir})
                message(FATAL_ERROR
                    "'${dir}'. The directory does not exist or it is a file.\n"
                    "Arg 'SOUCRES' of 'add_for_code_static_analize()' function "
                    "requires to specify directories if 'TARGET' is an interface library."
                )
            endif()
            list(APPEND dir_dests ${dir})
        endforeach()

        if(dir_dests)
            _import_include_directories_dependences(${ARG_TARGET} dir_dests)

            get_directory_property(target_dir_dests STATIC_ANALIZER_${ARG_TARGET}_DIRS)
            list(APPEND target_dir_dests ${dir_dests})
            list(REMOVE_DUPLICATES target_dir_dests)
            set_directory_properties(PROPERTIES 
                    STATIC_ANALIZER_${ARG_TARGET}_DIRS "${target_dir_dests}")

            set(glob_dests ${_STATIC_ANALIZER_ALL_DEST_DIRS} ${dir_dests})
            list(REMOVE_DUPLICATES glob_dests)
            set(_STATIC_ANALIZER_ALL_DEST_DIRS ${glob_dests} CACHE INTERNAL "")

            set(has_dest_files TRUE)
        endif()
    endif()

    if(has_dest_files)
        get_directory_property(curr_dir_targets STATIC_ANALIZER_TARGETS)
        list(APPEND curr_dir_targets ${ARG_TARGET})
        list(REMOVE_DUPLICATES curr_dir_targets)
        set_directory_properties(PROPERTIES STATIC_ANALIZER_TARGETS "${curr_dir_targets}")
    endif()
endfunction()

#
# exclude_from_code_static_analize
#
function(exclude_from_code_static_analize)
    if(NOT CLANG_TIDY)
        return()
    endif()
    if(NOT _STATIC_ANALIZER_ALL_DEST_FILES)
        return()
    endif()

    set(options)
    set(one_value_args TARGET)
    set(multi_value_args SOURCES)
    cmake_parse_arguments(ARG
        "${options}" "${one_value_args}" "${multi_value_args}"
        ${ARGN})
    if(ARG_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "Function 'exclude_from_code_static_analize()' called "
            "with unrecognized parameters: '${ARG_UNPARSED_ARGUMENTS}'")
    endif()

    if(NOT ARG_TARGET)
        message(WARNING "Target is empty")
        return()
    endif()

    _get_target_source_paths(${ARG_TARGET} src_path_list)
    if(NOT src_path_list)
        return()
    endif()

    get_directory_property(curr_dir_dests STATIC_ANALIZER_${ARG_TARGET}_FILES)

    set(glob_dests ${_STATIC_ANALIZER_ALL_DEST_FILES})
    if(NOT ARG_SOURCES)
        foreach(path ${src_path_list})
            list(REMOVE_ITEM curr_dir_dests ${path})
            list(REMOVE_ITEM glob_dests ${path})
        endforeach()
    else()
        foreach(src ${ARG_SOURCES})
            get_source_file_property(path ${src} LOCATION)

            list(FIND src_path_list ${path} idx)
            if(idx EQUAL -1)
                message(WARNING "File '${src}' doesn't belong target '${ARG_TARGET}'.")
            else()
                list(REMOVE_ITEM curr_dir_dests ${path})
                list(REMOVE_ITEM glob_dests ${path})
            endif()
        endforeach()
    endif()

    set_directory_properties(PROPERTIES 
            STATIC_ANALIZER_${ARG_TARGET}_FILES "${curr_dir_dests}")
    set(_STATIC_ANALIZER_ALL_DEST_FILES ${glob_dests} CACHE INTERNAL "")
endfunction()

function(_import_include_directories_dependences
    src_target
    src_files
)
    set(all_include_dirs)

    get_target_property(link_libs ${src_target} INTERFACE_LINK_LIBRARIES)
    foreach(lib ${link_libs})
        if (NOT TARGET ${lib})
            continue()
        endif()
        get_target_property(include_dirs ${lib} INTERFACE_INCLUDE_DIRECTORIES)
        if(include_dirs)
            list(APPEND all_include_dirs ${include_dirs})
        endif()
    endforeach()

    get_target_property(target_type ${src_target} TYPE)
    if (NOT (target_type STREQUAL INTERFACE_LIBRARY))
        get_target_property(link_libs ${src_target} LINK_LIBRARIES)

        foreach(lib ${link_libs})
            if (NOT TARGET ${lib})
                continue()
            endif()
            get_target_property(include_dirs ${lib} INTERFACE_INCLUDE_DIRECTORIES)
            if(include_dirs)
                list(APPEND all_include_dirs ${include_dirs})
            endif()
        endforeach()
        get_target_property(include_dirs ${src_target} INCLUDE_DIRECTORIES)
        list(APPEND all_include_dirs ${include_dirs})
    else()
        get_target_property(include_dirs ${src_target} INTERFACE_INCLUDE_DIRECTORIES)
        list(APPEND all_include_dirs ${include_dirs})
    endif()

    list(REMOVE_DUPLICATES all_include_dirs)

    set(all_include_opts)
    foreach(dir ${all_include_dirs})
        set(all_include_opts "${all_include_opts}-I${dir}&")
    endforeach()

    set(cmake_bin_gen_dir "${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}")
    set(clang_tidy_test_cpp "${cmake_bin_gen_dir}/clang-tidy_${src_target}.cpp")

    if (all_include_opts)
        set(updated_src_files)
        foreach(file ${${src_files}})
            if(NOT IS_DIRECTORY ${file})            
                string(REGEX MATCH ".+\\.h(pp)?" for_opt_matched ${file})
            else()
                set(for_opt_matched TRUE)
            endif()
            if(for_opt_matched)
                set(src_file_def "${file}?${all_include_opts}")
            else()
                set(src_file_def ${file})
            endif()
            list(APPEND updated_src_files ${src_file_def})
        endforeach()

        if (updated_src_files)
            set(${src_files} ${updated_src_files} PARENT_SCOPE)
        endif()
    endif()
endfunction()

function(_create_clang_tidy_make_target 
        make_target_name
        file_dests
        dir_dests)
    set(COMPILE_COMMANDS_FILE_DIR ${CMAKE_BINARY_DIR})

    set(CLANG_TIDY_CHECK_BUILD_DIR
        "${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/${make_target_name}.dir")
    set(CLANG_TIDY_CHECK_CACHE_DIR "${CLANG_TIDY_CHECK_BUILD_DIR}/cache")

    set(ct_chk_file ${CLANG_TIDY_CHECK_BUILD_DIR}/chk_files.make)
    set(ct_chk_file_content "chk_files := ${file_dests}\n\nchk_dirs := ${dir_dests}\n")
    file(GENERATE 
        OUTPUT ${ct_chk_file} 
        CONTENT "${ct_chk_file_content}"
        )

    set(ct_make_file 
        "${CLANG_TIDY_CHECK_BUILD_DIR}/Makefile")
    configure_file(
        ${ct_templ_make_file}
        ${ct_make_file}
        @ONLY
    )

    add_custom_target(${make_target_name}
        $(MAKE) -C ${CLANG_TIDY_CHECK_BUILD_DIR}
        COMMENT "Run clang-tidy checking"
        BYPRODUCTS ${CLANG_TIDY_CHECK_CACHE_DIR}
    )
endfunction()

#
# add_clang_tidy_check_target
#
function(add_clang_tidy_check_target)
    if(NOT CLANG_TIDY)
        return()
    endif()
    if((NOT _STATIC_ANALIZER_ALL_DEST_FILES) AND (NOT _STATIC_ANALIZER_ALL_DEST_DIRS))
        message(WARNING "Source files are not defined for the clang-tidy analize")
        return()
    endif()

    set(make_target_name "clang-tidy-check")

    get_directory_property(target_names STATIC_ANALIZER_TARGETS)

    foreach(target_name ${target_names})
        get_directory_property(file_dests STATIC_ANALIZER_${target_name}_FILES)
        get_directory_property(dir_dests STATIC_ANALIZER_${target_name}_DIRS)

        if((NOT file_dests) AND (NOT dir_dests))
            message(WARNING "Source files are not defined "
                    "for the clang-tidy analize in '${CMAKE_CURRENT_SOURCE_DIR}'")
            return()
        endif()

        set(local_make_target_name "${make_target_name}-${target_name}")

        _create_clang_tidy_make_target(
                ${local_make_target_name} 
                "${file_dests}" 
                "${dir_dests}"
                )
    endforeach()

    if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
        _create_clang_tidy_make_target(
                ${make_target_name} 
                "${_STATIC_ANALIZER_ALL_DEST_FILES}"
                "${_STATIC_ANALIZER_ALL_DEST_DIRS}"
                )
    endif()
endfunction()

