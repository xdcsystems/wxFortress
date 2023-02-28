@echo off
if errorlevel 1 goto end

set PATH=C:\cmake\bin;%;PATH%

del /Q CMakeCache.txt >nul 2>nul

cmake -G "Visual Studio 16 2019" -A Win32 -T host=x86 -S ..\wxArkanoid -B "x86"
rem cmake -G "Visual Studio 16 2019" -S ..\wxArkanoid -B "x64"

if errorlevel 1 goto end

:end
if errorlevel 1 (
  echo.
  echo.
  echo -------------------------------------------------
  echo ! Build FAILED with errors.                     !
  echo -------------------------------------------------
)

exit /b %errorlevel%
