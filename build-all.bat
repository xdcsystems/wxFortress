@echo off
setlocal EnableExtensions EnableDelayedExpansion

set "SRC_DIR=%~dp0"
if "%SRC_DIR:~-1%"=="\" set "SRC_DIR=%SRC_DIR:~0,-1%"

if not defined BUILD_DIR set "BUILD_DIR=%SRC_DIR%\..\wxFortress-build"
set "BUILD_TYPE=Release"
set "DO_CLEAN=0"
set "DO_RUN=0"
set "TOOLCHAIN=auto"

:parse_args
if "%~1"=="" goto :args_done
if /I "%~1"=="Release" ( set "BUILD_TYPE=Release" & shift & goto :parse_args )
if /I "%~1"=="Debug"   ( set "BUILD_TYPE=Debug"   & shift & goto :parse_args )
if /I "%~1"=="clean"   ( set "DO_CLEAN=1"         & shift & goto :parse_args )
if /I "%~1"=="run"     ( set "DO_RUN=1"           & shift & goto :parse_args )
if /I "%~1"=="mingw"   ( set "TOOLCHAIN=mingw"    & shift & goto :parse_args )
if /I "%~1"=="ninja"   ( set "TOOLCHAIN=mingw"    & shift & goto :parse_args )
if /I "%~1"=="vs"      ( set "TOOLCHAIN=vs"       & shift & goto :parse_args )
if /I "%~1"=="-h"      goto :usage
if /I "%~1"=="--help"  goto :usage
if /I "%~1"=="/?"      goto :usage
echo Unknown argument: %~1
goto :usage

:args_done
rem Optional user-supplied toolchain directory via env var (no hardcoded paths)
if defined TOOLCHAIN_BIN (
    if exist "!TOOLCHAIN_BIN!" set "PATH=!TOOLCHAIN_BIN!;!PATH!"
)

rem Fallback: if cmake or gcc is missing from PATH (and VS was not forced),
rem check sibling ..\mingw32\bin and standard Program Files CMake locations.
if /I not "%TOOLCHAIN%"=="vs" (
    where gcc >nul 2>&1
    if errorlevel 1 (
        if exist "%SRC_DIR%\..\mingw32\bin\gcc.exe" (
            set "PATH=%SRC_DIR%\..\mingw32\bin;!PATH!"
        )
    )
)

where cmake >nul 2>&1
if errorlevel 1 (
    if exist "%SRC_DIR%\..\mingw32\bin\cmake.exe" (
        set "PATH=%SRC_DIR%\..\mingw32\bin;!PATH!"
    ) else if exist "%ProgramFiles%\CMake\bin\cmake.exe" (
        set "PATH=%ProgramFiles%\CMake\bin;!PATH!"
    ) else if exist "%ProgramFiles(x86)%\CMake\bin\cmake.exe" (
        set "PATH=%ProgramFiles(x86)%\CMake\bin;!PATH!"
    )
)

where cmake >nul 2>&1
if errorlevel 1 (
    echo [ERROR] cmake not found on PATH.
    echo Add your toolchain bin directory to PATH or set TOOLCHAIN_BIN before running build-all.bat.
    goto :fail
)

set "TOOLCHAIN_EXPLICIT=1"
if /I "%TOOLCHAIN%"=="auto" (
    set "TOOLCHAIN_EXPLICIT=0"
    set "HAS_MINGW=1"
    where ninja >nul 2>&1 || set "HAS_MINGW=0"
    where gcc   >nul 2>&1 || set "HAS_MINGW=0"
    where g++   >nul 2>&1 || set "HAS_MINGW=0"
    if "!HAS_MINGW!"=="1" (
        set "TOOLCHAIN=mingw"
    ) else (
        set "TOOLCHAIN=vs"
    )
)

if "%DO_CLEAN%"=="1" (
    if exist "%BUILD_DIR%" (
        echo Cleaning "%BUILD_DIR%" ...
        rmdir /S /Q "%BUILD_DIR%" || goto :fail
    )
)

set "NEED_CONFIGURE=0"
if not exist "%BUILD_DIR%\CMakeCache.txt" (
    set "NEED_CONFIGURE=1"
) else (
    rem Detect cached generator mismatch when toolchain is explicitly requested
    findstr /B /C:"CMAKE_GENERATOR:INTERNAL=Ninja" "%BUILD_DIR%\CMakeCache.txt" >nul 2>&1
    if not errorlevel 1 (
        if "%TOOLCHAIN_EXPLICIT%"=="1" if /I "%TOOLCHAIN%"=="vs" (
            echo [ERROR] "%BUILD_DIR%" is already configured for Ninja. Re-run with "clean" to switch to Visual Studio.
            goto :fail
        )
        rem Single-config Ninja: reconfigure if Debug/Release changed
        findstr /B /C:"CMAKE_BUILD_TYPE:STRING=%BUILD_TYPE%" "%BUILD_DIR%\CMakeCache.txt" >nul 2>&1
        if errorlevel 1 set "NEED_CONFIGURE=1"
    ) else (
        if "%TOOLCHAIN_EXPLICIT%"=="1" if /I "%TOOLCHAIN%"=="mingw" (
            echo [ERROR] "%BUILD_DIR%" is already configured for a non-Ninja generator. Re-run with "clean" to switch to MinGW/Ninja.
            goto :fail
        )
    )
)

if "%NEED_CONFIGURE%"=="1" (
    echo Configuring wxFortress [%BUILD_TYPE%] using toolchain: %TOOLCHAIN%
    if /I "%TOOLCHAIN%"=="mingw" (
        where ninja >nul 2>&1 || ( echo [ERROR] ninja not found on PATH. & goto :fail )
        where gcc   >nul 2>&1 || ( echo [ERROR] gcc not found on PATH.   & goto :fail )
        where g++   >nul 2>&1 || ( echo [ERROR] g++ not found on PATH.   & goto :fail )
        cmake -S "%SRC_DIR%" -B "%BUILD_DIR%" -G Ninja ^
            -DCMAKE_BUILD_TYPE=%BUILD_TYPE% ^
            -DCMAKE_POLICY_VERSION_MINIMUM=3.5 ^
            -DCMAKE_C_COMPILER=gcc ^
            -DCMAKE_CXX_COMPILER=g++ || goto :fail
    ) else (
        cmake -S "%SRC_DIR%" -B "%BUILD_DIR%" -A Win32 -T host=x86 ^
            -DCMAKE_POLICY_VERSION_MINIMUM=3.5 || goto :fail
    )
)

echo Building wxFortress and running GoogleTest suites [%BUILD_TYPE%] ...
cmake --build "%BUILD_DIR%" --config %BUILD_TYPE% --parallel || goto :fail

set "EXE_DIR="
if exist "%BUILD_DIR%\.bin\wxFortress.exe" (
    set "EXE_DIR=%BUILD_DIR%\.bin"
) else if exist "%BUILD_DIR%\.bin\%BUILD_TYPE%\wxFortress.exe" (
    set "EXE_DIR=%BUILD_DIR%\.bin\%BUILD_TYPE%"
)

echo.
echo Build and tests PASSED.
if defined EXE_DIR echo Binary: "!EXE_DIR!\wxFortress.exe"

if "%DO_RUN%"=="1" (
    if defined EXE_DIR (
        pushd "!EXE_DIR!"
        start "" "wxFortress.exe"
        popd
    ) else (
        echo [ERROR] Built wxFortress.exe not found under "%BUILD_DIR%\.bin".
        goto :fail
    )
)

exit /b 0

:usage
echo Usage: build-all.bat [Release^|Debug] [mingw^|vs] [clean] [run]
echo Optional env vars: BUILD_DIR, TOOLCHAIN_BIN
exit /b 2

:fail
echo.
echo -------------------------------------------------
echo ! Build FAILED with errors.                     !
echo -------------------------------------------------
exit /b 1
