#
#   Используется для получения исходных кодов внешних библиотек, их компиляции и установки
#   --------------------------------------------------------------------------------------
#
#
# Make цели:
#   src - импортировать исходные коды библиотек
#   build - собрать и скомпилировать рекурсивно библиотеки
#   install - выполнить инсталляцию
#
# Make переменные (определяются через аргументы командной строки вызова утилиты make):
#    SRC_DIR - базовый каталог, куда будет выполнен импорт исходных кодов библиотек.
#       ОБЯЗАТЕЛЕН для определения. 
#       Используется во всех целях.
#    INSTALL_DIR - каталог, куда будет выполнена инсталляция библиотек. 
#       Если не указан, путь инсталляции будет определяться индивидуальными настройками библиотек
#       Используется только в цели build.
#    NARROW_BUILD - используется только для цели build. 
#       Принимает значения:
#         0 - выполнять проверку зависимостей (пререквизитов) от исходных файлов. Значение по умолчанию.
#         1 - не выполнять проверку зависимостей (пререквизитов) от исходных файлов.
#
#
# Обязательный локальный подключаемый файл extern_dep.mk должен содержать описание подключения внешних библиотек.
# Описание создается путем вызова функций apt_get, git_build_install, git_build (см. ниже)
#
#
# Примеры вызова утилиты make: 
#   1. make SRC_DIR=./lib/src src
#   2. make SRC_DIR=./lib/src INSTALL_DIR=./lib/opt build
#   3. make SRC_DIR=./lib/src install
#   4. make SRC_DIR=./lib/src INSTALL_DIR=./lib/opt NARROW_BUILD=1 build install
#
#------------------------------------------------------------------------------------------

#
#  Функция get_src_dir
# ---------------------
#
# Формирует название каталога исходных кодов проекта
#
# Параметры:
# (1) - название проекта
# (2) - версия проекта
#
define get_src_dir =
$(abspath $(SRC_DIR))/$(strip $(1))$(if $(strip $(2)),-$(strip $(2)),)
endef


#
#  Шаблон-функция src_from_git_target_template
# ---------------------------------------------
#
# Создает make-цель для получения проекта из git-репозитория
#
# Параметры:
# (1) - название проекта
# (2) - версия проекта
# (3) - URL git-репозитория
# (4) - аргументы вызова git clone 
# (5) - ветка git-репозитория
#

define src_from_git_target_template =
$(1)_name := $$(strip $(1))
ifeq ($$($(1)_name),)
    $$(error "The library name is empty")
endif

$(1)_vers := $(if $(strip $(2)),$(strip $(2)),0.0)

ifeq ($$(strip $(3)),)
    $$(error "The Git source path is empty")
endif
ifeq ($$(strip $(5)),)
    $$(error "The Git branch is empty")
endif

$(1)_src_dir := $$(call get_src_dir,$$($(1)_name),$$($(1)_vers))
$(1)_src_ref_dir := $$(call get_src_dir,$$($(1)_name),)

$(1)_src_target := $$($(1)_name)_src
.PHONY: $$($(1)_src_target) 

$$($(1)_src_target): 
	@src_dir="$$($(1)_src_dir)"; \
	src_ref_dir="$$($(1)_src_ref_dir)"; \
	name="$$($(1)_name)"; \
	vers="$$($(1)_vers)"; \
	git_src="$(3)"; \
	git_clone_args="$(4)"; \
	git_branch="$(5)"; \
	need_new=0; \
	version_matched=0; \
	if [ ! -d "$$$${src_ref_dir}" ]; then \
		src_base_dir=$$$$(dirname $$$${src_ref_dir}); \
		if [ $$$$? -ne 0 ]; then \
			exit 1; \
		fi; \
		mkdir -p "$$$${src_base_dir}"; \
		if [ $$$$? -ne 0 ]; then \
			exit 1; \
		fi; \
		need_new=1; \
	else \
		real_dir=$$$$(readlink $$$${src_ref_dir}); \
		if [ $$$$? -ne 0 ]; then \
			echo "Error link reading from $$$${src_ref_dir}"; \
			exit $$$$?; \
		fi; \
		curr_vers=$$$${real_dir#$$$${name}-}; \
		curr_major_num=$$$$(($$$$(echo $$$${curr_vers} | awk -F. '{print $$$$1}'))); \
		if [ -z "$$$${curr_major_num}" ]; then \
			curr_major_num=0; \
		fi; \
		curr_minor_num=$$$$(echo $$$${curr_vers} | awk -F. '{print $$$$2}'); \
		if [ -z "$$$${curr_minor_num}" ]; then \
			curr_minor_num=0; \
		fi; \
		curr_tweak_num=$$$$(echo $$$${curr_vers} | awk -F. '{print $$$$3}'); \
		if [ -z "$$$${curr_tweak_num}" ]; then \
			curr_tweak_num=0; \
		fi; \
		new_major_num=$$$$(echo $$$${vers} | awk -F. '{print $$$$1}'); \
		if [ -z "$$$${new_major_num}" ]; then \
			new_major_num=0; \
		fi; \
		new_minor_num=$$$$(echo $$$${vers} | awk -F. '{print $$$$2}'); \
		if [ -z "$$$${new_minor_num}" ]; then \
			new_minor_num=0; \
		fi; \
		new_tweak_num=$$$$(echo $$$${vers} | awk -F. '{print $$$$3}'); \
		if [ -z "$$$${new_tweak_num}" ]; then \
			new_tweak_num=0; \
		fi; \
		if [ $$$${new_major_num} -gt $$$${curr_major_num} ]; then \
			need_new=1; \
		elif [ $$$${new_major_num} -eq $$$${curr_major_num} ]; then \
			if [ $$$${new_minor_num} -gt $$$${curr_minor_num} ]; then \
				need_new=1; \
			elif [ $$$${new_minor_num} -eq $$$${curr_minor_num} ]; then \
				if [ $$$${new_tweak_num} -gt $$$${curr_tweak_num} ]; then \
					need_new=1; \
				fi; \
			fi; \
		fi; \
		if [ "$$$${curr_vers}" = "$$$${vers}" ]; then \
			version_matched=1; \
		fi; \
	fi; \
	if [ "$$$${need_new}" -ne 0 ]; then \
		if [ -e $$$${src_dir} ]; then \
			mv $$$${src_dir} "$$$${src_dir}.$$$$(date +%s)"; \
		fi; \
		git_clone_cmd="git clone $$$${git_src} -b $$$${git_branch} $$$${git_clone_args} $$$${src_dir}"; \
		printf "\nExecute: %s\n" "$$$${git_clone_cmd}"; \
		eval $$$${git_clone_cmd}; \
		if [ $$$$? -ne 0 ]; then \
			exit 1; \
		fi; \
		if [ -e $$$${src_ref_dir} ]; then \
			unlink $$$${src_ref_dir}; \
		fi; \
		ln -T -s -r -f $$$${src_dir} $$$${src_ref_dir}; \
		if [ $$$$? -ne 0 ]; then \
			exit 1; \
		fi; \
	else \
		if [ "$$(UPDATE_SRC)" != 0 ]; then \
			if [ "$$$${version_matched}" -eq 0 ]; then \
				exit 0; \
			fi; \
			cd $$$${src_dir}; \
			if [ $$$$? != 0 ]; then \
				exit 1; \
			fi; \
			git_pull_cmd="git pull $$$${git_src} $$$${git_branch}"; \
			printf "\nExecute: %s\n" "$$$${git_pull_cmd}"; \
			eval $$$${git_pull_cmd}; \
			if [ $$$$? != 0 ]; then \
				exit 1; \
			fi; \
			cd - &>/dev/null; \
		fi; \
	fi; \
	cmake_cache_file="$$$${src_dir}/build/CMakeCache.txt"; \
	if [ -f "$$$${cmake_cache_file}" ]; then \
		rm $$$${cmake_cache_file}; \
	fi; \
	extern_dep_creator_file=$$$${src_dir}/dev_tool/extern_lib_deploy/generate_deps.sh; \
	if [ -f "$$$${src_dir}/extern_dep_config.cmake" -a -f "$$$${extern_dep_creator_file}" ]; then \
		$$$${extern_dep_creator_file} $$$${src_dir}; \
		if [ $$$$? -ne 0 ]; then \
			exit 1; \
		fi; \
	fi; \
	extern_dep_dir=$$$${src_dir}/extern_dep; \
	if [ -d "$$$${extern_dep_dir}" ]; then \
		$$(MAKE) -C $$$${extern_dep_dir} src; \
	fi 

SRC_TARGETS := $$(SRC_TARGETS) $$($(1)_src_target)
endef


#
#  Функция src_from_git_target
# -----------------------------
#
# Создает make-цель для получения проекта из git-репозитория
#
# Параметры:
# (1) - название проекта
# (2) - версия проекта
# (3) - URL git-репозитория
# (4) - аргументы вызова git clone 
# (5) - ветка git-репозитория
#

define src_from_git_target = 
$(eval $(call src_from_git_target_template,$(strip $(1)),$(strip $(2)),$(strip $(3)),$(strip $(4)),$(strip $(5))))
endef


#
#  Шаблон-Функция cmake_build_target_template
# --------------------------------------------
#
# Создает make-цель для сборки проекта посредством CMake и make утилит
#
# Параметры:
# (1) - название проекта
# (2) - аргументы командной строки утилиты CMake
#

define cmake_build_target_template =
$(1)_name := $$(strip $(1))
ifeq ($$($(1)_name),)
    $$(error "The library name is empty")
endif

$(1)_src_ref_dir := $$(call get_src_dir,$$($(1)_name),)

ifndef NARROW_BUILD
    $(1)_src_target := $$($(1)_name)_src
else
    ifeq ($$(NARROW_BUILD), 0)
        $(1)_src_target := $$($(1)_name)_src
    else
        $(1)_src_target :=
    endif
endif

$(1)_build_target := $$($(1)_name)_build

.PHONY: $$($(1)_build_target)

$$($(1)_build_target): $$($(1)_src_target)
	@src_dir="$$($(1)_src_ref_dir)"; \
	if [ ! -d "$$$${src_dir}" ]; then \
		echo "Target $$@: the source directory $$$${src_dir} does not exist"; \
		exit 1; \
	fi; \
	extern_dep_dir=$$$${src_dir}/extern_dep; \
	if [ -d "$$$${extern_dep_dir}" ]; then \
		$$(MAKE) -C $$$${extern_dep_dir} build; \
		if [ $$$$? -ne 0 ]; then \
			exit 1; \
		fi; \
		need_sudo=0; \
		if [ -n "$$(INSTALL_DIR)" ]; then \
			curr_user=$$$$(whoami); \
			install_dir_owner=$$$$(stat -c%G "$$(INSTALL_DIR)" 2>/dev/null); \
			if [ "$$$${curr_user}" != "$$$${install_dir_owner}" ]; then \
				need_sudo=1; \
			fi; \
		else \
			need_sudo=1; \
		fi; \
		if [ $$$${need_sudo} -ne 0 ]; then \
			sudo -S -E $$(MAKE) -C $$$${extern_dep_dir} install; \
		else \
			$$(MAKE) -C $$$${extern_dep_dir} install; \
		fi; \
		if [ $$$$? -ne 0 ]; then \
			exit 1; \
		fi; \
	fi; \
	build_dir="$$$${src_dir}/build"; \
	cmake_args="$(2) -DCMAKE_FIND_PACKAGE_NO_PACKAGE_REGISTRY=TRUE"; \
	if [ -n "$$(INSTALL_DIR)" ]; then \
		cmake_args="-DCMAKE_PREFIX_PATH=$$(INSTALL_DIR) -DCMAKE_INSTALL_PREFIX=$$(INSTALL_DIR) $$$${cmake_args}"; \
	fi; \
	mkdir -p "$$$${build_dir}" && \
	cd "$$$${build_dir}"; \
	if [ $$$$? -ne 0 ]; then \
		exit 1; \
	fi; \
	cmd="cmake $$$${cmake_args} $$$${src_dir}"; \
	printf "\nExecute: %s\n" "$$$${cmd}"; \
	eval $$$${cmd} && \
	$$(MAKE) -j`nproc` && \
	ctest --output-on-failure .

BUILD_TARGETS := $$(BUILD_TARGETS) $$($(1)_build_target)
endef


#
#  Функция cmake_build_target
# ----------------------------
#
# Создает make-цель для сборки проекта посредством CMake и make утилит
#
# Параметры:
# (1) - название проекта
# (2) - аргументы командной строки утилиты CMake
#

define cmake_build_target = 
$(eval $(call cmake_build_target_template,$(strip $(1)),$(strip $(2))))
endef


#
#  Шаблон-функция custom_build_target_template
# --------------------------------------------
#
# Создает make-цель для сборки проекта посредством его специфических команд сборки
#
# Параметры:
# (1) - название проекта
# (2) - команды сборки проекта
#

define custom_build_target_template =
$(1)_name := $$(strip $(1))
ifeq ($$($(1)_name),)
    $$(error "The library name is empty")
endif

$(1)_src_ref_dir := $$(call get_src_dir,$$($(1)_name),)

ifndef NARROW_BUILD
    $(1)_src_target := $$($(1)_name)_src
else
    ifeq ($$(NARROW_BUILD), 0)
        $(1)_src_target := $$($(1)_name)_src
    else
        $(1)_src_target :=
    endif
endif

$(1)_build_target := $$($(1)_name)_build

.PHONY: $$($(1)_build_target)

$$($(1)_build_target): $$($(1)_src_target)
	@src_dir="$$($(1)_src_ref_dir)"; \
	if [ ! -d "$$$${src_dir}" ]; then \
		echo "Target $$@: the source directory $$$${src_dir} does not exist"; \
		exit 1; \
	fi; \
	cd "$$$${src_dir}"; \
	if [ $$$$? -ne 0 ]; then \
		exit 1; \
	fi; \
	build_cmds=$(2); \
	printf "\nExecute: %s\n" "$$$${build_cmds}"; \
	eval $$$${build_cmds}; \
	if [ $$$$? -ne 0 ]; then \
		exit 1; \
	fi; 

BUILD_TARGETS := $$(BUILD_TARGETS) $$($(1)_build_target)
endef


#
#  Функция custom_build_target
# -----------------------------
#
# Создает make-цель для сборки проекта посредством его специфических команд сборки
#
# Параметры:
# (1) - название проекта
# (2) - команды сборки проекта
#

define custom_build_target =
$(eval $(call custom_build_target_template,$(strip $(1)),$(strip $(2))))
endef


#
#  Шаблон-функция make_install_target_template
# ----------------------------------------------
#
# Создает make-цель для инсталляции проекта через вызов make install
#
# Параметры:
# (1) - каталог исходных кодов проекта
#

define make_install_target_template =
ifeq ($$(strip $(1)),"")
	$$(error "The library source path is empty")
endif

.PHONY: $$(notdir $(1))_install

$$(notdir $(1))_install:
	@src_dir="$(1)"; \
	if [ ! -d "$$$${src_dir}" ]; then \
		echo "Target $$@: the source directory is undefined"; \
		exit 1; \
	fi; \
	build_dir="$$$${src_dir}/build"; \
	if [ ! -d "$$$${build_dir}" ]; then \
		echo "Target $$@: the build directory doen't exist. Installation is ignored"; \
		exit 0; \
	fi; \
	cmd="$$(MAKE) -C $$$${build_dir} install"; \
	printf "\nExecute: %s\n" "$$$${cmd}"; \
	eval $$$${cmd}

INSTALL_TARGETS += $$(notdir $(1))_install 
endef


#
#  Функция make_install_target
# -----------------------------
#
# Создает make-цель для инсталляции проекта через вызов make install
#
# Параметры:
# (1) - название проекта
# (2) - версия проекта
#

define make_install_target = 
$(eval $(call make_install_target_template,$(call get_src_dir,$(1),)))
endef


#
#  Шаблон-функция custom_install_target_template
# -----------------------------------------------
#
# Создает make-цель для инсталляции проекта через вызов его специфических команд инсталляции
#
# Параметры:
# (1) - каталог исходных кодов проекта
# (2) - команды инсталляции проекта
#

define custom_install_target_template =
ifeq ($$(strip $(1)),"")
	$$(error "The library source path is empty")
endif

.PHONY: $$(notdir $(1))_install

$$(notdir $(1))_install:
	@src_dir="$(1)"; \
	if [ ! -d "$$$${src_dir}" ]; then \
		echo "Target $$@: the source directory is undefined"; \
		exit 1; \
	fi; \
	cd $$$${src_dir}; \
	if [ $$$$? -ne 0 ]; then \
		exit 1; \
	fi; \
	install_cmds=$(2); \
	printf "\nExecute: %s\n" "$$$${install_cmds}"; \
	eval $$$${install_cmds}; \
	if [ $$$$? -ne 0 ]; then \
		exit 1; \
	fi; 

INSTALL_TARGETS += $$(notdir $(1))_install 
endef


#
#  Функция custom_install_target
# -------------------------------
#
# Создает make-цель для инсталляции проекта через вызов его специфических команд инсталляции
#
# Параметры:
# (1) - название проекта
# (2) - версия проекта
# (3) - команды инсталляции проекта
#

define custom_install_target =
$(eval $(call custom_install_target_template,$(call get_src_dir,$(1),$(2)),$(strip $(3))))
endef


#
#  Шаблон-функция git_build_targets_template
# -------------------------------------------
#
# Создает make-цели для 
#  - получения проекта из git-репозитория;
#  - сборки проекта посредством CMake и make утилит
#
# Параметры:
# (1) - название проекта
# (2) - версия проекта
# (3) - URL git-репозитория
# (4) - аргументы вызова git clone 
# (5) - ветка git-репозитория
# (6) - аргументы командной строки утилиты CMake
#

define git_build_targets_template = 
$$(call src_from_git_target,$(1),$(2),$(3),$(4),$(5),$(6))
$$(call cmake_build_target,$(1),$(6))
endef


#
#  Функция git_build
# -------------------
#
# Создает make-цели для 
#  - получения проекта из git-репозитория;
#  - сборки проекта посредством CMake и make утилит
#
# Параметры:
# (1) - название проекта
# (2) - версия проекта
# (3) - URL git-репозитория
# (4) - аргументы вызова git clone 
# (5) - ветка git-репозитория
# (6) - аргументы командной строки утилиты CMake
#
define git_build = 
$(eval $(call git_build_targets_template,$(strip $(1)),$(strip $(2)),$(strip $(3)),$(strip $(4)),$(strip $(5)),$(strip $(6))))
endef


#
#  Функция git_build_install
# ---------------------------
#
# Создает make-цели для 
#  - получения проекта из git-репозитория;
#  - сборки проекта посредством CMake и make утилит;
#  - инсталляции проекта через вызов make install
#
# Параметры:
# (1) - название проекта
# (2) - версия проекта
# (3) - URL git-репозитория
# (4) - аргументы вызова git clone 
# (5) - ветка git-репозитория
# (6) - аргументы командной строки утилиты CMake
#

define git_build_install = 
$(call git_build,$(1),$(2),$(3),$(4),$(5),$(6));
$(call make_install_target,$(1),$(2))
endef


#
#  Функция git_custom_build_install
# ----------------------------------
#
# Создает make-цели для 
#  - получения проекта из git-репозитория;
#  - сборки проекта путем вызова специфических команд сборки  
#  - инсталляции проекта через вызов специфических команд установки
#
# Параметры:
# (1) - название проекта
# (2) - версия проекта
# (3) - URL git-репозитория
# (4) - аргументы вызова git clone 
# (5) - ветка git-репозитория
# (6) - команды shell-скрипта для сборки проекта
# (7) - команды shell-скрипта для установки проекта 
#

define git_custom_build_install = 
$(call src_from_git_target,$(1),$(2),$(3),$(4),$(5));
$(call custom_build_target,$(1),$(6));
$(call custom_install_target,$(1),$(2),$(7))
endef


# 
#  Функция apt_packages
# ----------------------
#
# Добавляет APT-пакет в список инсталляции
#
# Параметры:
# (1) - название APT-пакета
#

define apt_packages = 
$(eval APT_PACKAGE_TARGETS += $(strip $(1)))
endef


#
### Основная реализация
#
SHELL := /bin/bash

# ${SRC_DIR} - каталог для хранения внешних проектов и сборок
ifndef SRC_DIR
    $(error SRC_DIR variable for storing source codes must be defined)
endif

SRC_TARGETS :=
APT_PACKAGE_TARGETS :=
BUILD_TARGETS :=

ifndef UPDATE_SRC
    UPDATE_SRC = 1
else
    ifneq ($$(UPDATE_SRC), 0)
        UPDATE_SRC = 1
    endif
endif

.PHONY: build apt_get install

all: build 

# Файл extern_dep.mk создается под конкретный проект на основании внешних
# библиотек, используемых проектом. Содержимое файла формируется на основании вызовов 
# git_build_install, git_build, apt_get 
#
include extern_dep.mk

src: $(SRC_TARGETS)

build: apt_get $(BUILD_TARGETS)

apt_get:
	@if [ -n "$(APT_PACKAGE_TARGETS)" ]; then \
		for package_decl in $(APT_PACKAGE_TARGETS); do \
			if [ "$${package_decl}" = "$${package_decl%%\?}" ]; then \
				packages=$$(echo $${package_decl} | sed -e "s/:/ /g"); \
				installed=0; \
				for pkg in $${packages}; do \
					apt show $${pkg} &>/dev/null; \
					if [ $$? -ne 0 ]; then \
						continue; \
					fi; \
					dpkg-query -s $${pkg} &>/dev/null; \
					if [ $$? -eq 0 ]; then \
						installed=1; \
						continue; \
					fi; \
					shopt -s xpg_echo; \
					printf "\n Установка APT пакета %s\n" "$${pkg}"; \
					sudo -S DEBIAN_FRONTEND="noninteractive" apt-get install -y -q $${pkg}; \
					if [ $$? -ne 0 ]; then \
						exit 1; \
					fi; \
					installed=1; \
					break; \
				done; \
				if [ "$${installed}" = 0 ]; then \
					echo "Ни один пакет $${packages} не найден в репозитории APT"; \
					exit 1; \
				fi; \
			else \
				pkg=$${package_decl%%\?}; \
				apt show $${pkg} &>/dev/null; \
				if [ $$? -ne 0 ]; then \
					printf "\nAPT пакет %s игнорируется\n" "$${pkg}"; \
					continue; \
				fi; \
				dpkg-query -s $${pkg} &>/dev/null; \
				if [ $$? -eq 0 ]; then \
					continue; \
				fi; \
				printf "\nУстановка APT пакета %s\n" "$${pkg}"; \
				sudo -S DEBIAN_FRONTEND="noninteractive" apt-get install -y -q $${pkg}; \
				if [ $$? -ne 0 ]; then \
					exit 1; \
				fi; \
			fi; \
		done; \
	fi 

install: $(INSTALL_TARGETS)

