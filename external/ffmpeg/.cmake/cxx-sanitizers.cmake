#[=============================================================================[

Добавляет через cmake-опции в сборку проекта опции компилятора C++/C для проверки:
    - неинициализированных значений переменных: SANITIZE_UNDEFINED
    - консистентности оперативной памяти: SANITIZE_ADDRESS
    - утечек памяти: SANITIZE_LEAK
    - объектов синхронизации потоков и гонки данных: SANITIZE_THREAD

#]=============================================================================]

cmake_minimum_required(VERSION 3.7)

include(helpers)
ROOT_INCLUDE_GUARD()

option(SANITIZE_UNDEFINED
    "Enable C++ UndefinedBehaviorSanitizer, a fast undefined behavior detector" OFF)
option(SANITIZE_ADDRESS
    "Enable C++ AddressSanitizer, a fast memory error detector" OFF)
option(SANITIZE_THREAD
    "Enable C++ ThreadSanitizer, a fast data race detector" OFF)
option(SANITIZE_LEAK
"Enable C++ LeakSanitizer, a memory leak detector. \
This option only matters for linking of executables and \
if neither SANITIZE_ADDRESS or SANITIZE_THREAD is option used." OFF)

option(SANITIZE_DEFAULT
    "Enable C++ UndefinedBehaviorSanitizer and AddressSanitizer" OFF)

if (SANITIZE_UNDEFINED OR DEFAULT_SANITIZATION)
    set(_sanitize_undef_flags " -DSANITIZE_UNDEFINED -fsanitize=undefined")

    string(APPEND CMAKE_CXX_FLAGS "${_sanitize_undef_flags}")
    string(APPEND CMAKE_C_FLAGS "${_sanitize_undef_flags}")

    message("Enabled C++ UndefinedBehaviorSanitizer")
endif()

if (SANITIZE_ADDRESS OR DEFAULT_SANITIZATION)
    set(_sanitize_addr_flags " -DSANITIZE_ADDRESS -fsanitize=address")

    string(APPEND CMAKE_CXX_FLAGS "${_sanitize_addr_flags}")
    string(APPEND CMAKE_C_FLAGS "${_sanitize_addr_flags}")

    message("Enabled C++ AddressSanitizer")
endif()

if (SANITIZE_LEAK)
    set(_sanitize_leak_flags " -DSANITIZE_LEAK -fsanitize=leak")

    string(APPEND CMAKE_CXX_FLAGS "${_sanitize_leak_flags}")
    string(APPEND CMAKE_C_FLAGS "${_sanitize_leak_flags}")

    message("Enabled C++ LeakSanitizer")
endif()

if (SANITIZE_THREAD)
    set(_sanitize_thread_flags " -DSANITIZE_THREAD -fsanitize=thread")

    string(APPEND CMAKE_CXX_FLAGS "${_sanitize_thread_flags}")
    string(APPEND CMAKE_C_FLAGS "${_sanitize_thread_flags}")

    message("Enabled C++ ThreadSanitizer")
endif()
