#[=============================================================================[

Подключает к проекту часто используемые cmake-модули
для создания приложений по стандарту, используемому
внутри компании.

#]=============================================================================]

cmake_minimum_required(VERSION 3.7)

include(helpers)
ROOT_INCLUDE_GUARD()

include(cxx-project)
include(extern_dep)
include(set-target-export-properties)
include(create-package)
include(gsl)
include(clang-tidy)
include(test)
include(gtest)
include(doxygen)
