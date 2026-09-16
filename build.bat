@echo off
REM PvZ-Portable Windows build script (MSYS2 UCRT64 + Ninja)
REM
REM Usage: build.bat [release|debug] [clean]
REM
REM   release   Release build, standalone static exe (default)
REM   debug     Debug build with console window and assertions
REM   clean     delete the build directory before configuring
REM
REM Environment overrides:
REM   MSYS2_ROOT           MSYS2 installation directory (default: C:\msys64)
REM   BUILD_DIR            build directory (default: build)
REM   CMAKE_GENERATOR      CMake generator (default: Ninja)

setlocal EnableDelayedExpansion
pushd "%~dp0" || (
	echo Error: cannot enter the repository directory.
	pause
	exit /b 1
)

set "BUILD_TYPE=Release"
set "DO_CLEAN="

REM Pause before closing the window when the script was launched by double-click.
set "IS_DOUBLECLICK="
echo "%CMDCMDLINE%" | find /i " /c " >nul || set "IS_DOUBLECLICK=1"

REM --- Parse arguments -------------------------------------------------------
:parse_args
if "%~1"=="" goto args_done
if /i "%~1"=="release" (
	set "BUILD_TYPE=Release"
) else if /i "%~1"=="debug" (
	set "BUILD_TYPE=Debug"
) else if /i "%~1"=="clean" (
	set "DO_CLEAN=1"
) else (
	echo Error: unknown argument "%~1".
	echo.
	echo Usage: build.bat [release^|debug] [clean]
	popd
	endlocal
	exit /b 2
)
shift
goto parse_args
:args_done

set "BUILD_STATIC=ON"
if /i "%BUILD_TYPE%"=="Debug" (
	set "CONSOLE=ON"
) else (
	set "CONSOLE=OFF"
)

if not defined BUILD_DIR set "BUILD_DIR=build"
if not defined CMAKE_GENERATOR set "CMAKE_GENERATOR=Ninja"

echo ==========================================================
echo  PvZ-Portable Windows build
echo    Build type : %BUILD_TYPE%
echo    Static     : %BUILD_STATIC%
echo    Console    : %CONSOLE%
echo    Build dir  : %BUILD_DIR%
echo    Generator  : %CMAKE_GENERATOR%
echo ==========================================================
echo.

REM --- Locate MSYS2 ----------------------------------------------------------
set "MSYS2_ROOT_MSG=not detected"
if defined MSYS2_ROOT set "MSYS2_ROOT_MSG=%MSYS2_ROOT%"
if not defined MSYS2_ROOT if defined MSYS2_HOME set "MSYS2_ROOT=%MSYS2_HOME%"
if not defined MSYS2_ROOT if defined MINGW_PREFIX for %%I in ("%MINGW_PREFIX%\..\..") do set "MSYS2_ROOT=%%~fI"
REM Accept a candidate root only when it really contains a UCRT64 toolchain.
if defined MSYS2_ROOT if not exist "%MSYS2_ROOT%\ucrt64\bin\gcc.exe" set "MSYS2_ROOT="
if not defined MSYS2_ROOT if exist "C:\msys64\ucrt64\bin\gcc.exe" set "MSYS2_ROOT=C:\msys64"
if not defined MSYS2_ROOT if exist "D:\msys64\ucrt64\bin\gcc.exe" set "MSYS2_ROOT=D:\msys64"
if defined MSYS2_ROOT set "MSYS2_ROOT_MSG=%MSYS2_ROOT%"

set "UCRT64_BIN=%MSYS2_ROOT%\ucrt64\bin"
set "USR_BIN=%MSYS2_ROOT%\usr\bin"
set "UCRT64_LIB=%MSYS2_ROOT%\ucrt64\lib"
set "UCRT64_INC=%MSYS2_ROOT%\ucrt64\include"
set "CMAKE_EXE=%UCRT64_BIN%\cmake.exe"
set "CMAKE_STRIP=%UCRT64_BIN%\strip.exe"
set "PACMAN_PACKAGES=base-devel mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-ninja mingw-w64-ucrt-x86_64-libjpeg-turbo mingw-w64-ucrt-x86_64-libopenmpt mingw-w64-ucrt-x86_64-libogg mingw-w64-ucrt-x86_64-libpng mingw-w64-ucrt-x86_64-libvorbis mingw-w64-ucrt-x86_64-mpg123 mingw-w64-ucrt-x86_64-SDL2"

if not exist "%CMAKE_EXE%" goto err_no_msys2

REM --- Check required tools --------------------------------------------------
set "MISSING="
for %%T in (cmake.exe ninja.exe gcc.exe g++.exe windres.exe strip.exe ar.exe) do (
	if not exist "%UCRT64_BIN%\%%T" set "MISSING=!MISSING! %%T"
)
if not exist "%USR_BIN%\msys-2.0.dll" set "MISSING=!MISSING! msys-2.0.dll"
if defined MISSING goto err_no_tools

REM --- Use the UCRT64 toolchain ----------------------------------------------
REM Clear variables that a MSYS2 shell would export, then point PATH at UCRT64
REM and at the MSYS runtime that cmake.exe and ninja.exe need.
set "CC="
set "CXX="
set "CFLAGS="
set "CXXFLAGS="
set "PATH=%UCRT64_BIN%;%USR_BIN%;%PATH%"

REM --- Check dependencies ----------------------------------------------------
REM SDL2 and Ogg are found with CMake config files; the others are detected from
REM their headers and import libraries, so probe both.
set "MISSING="
for %%F in (
	"%UCRT64_LIB%\cmake\SDL2\SDL2Config.cmake"
	"%UCRT64_LIB%\cmake\Ogg\OggConfig.cmake"
	"%UCRT64_LIB%\libSDL2.dll.a"
	"%UCRT64_LIB%\libSDL2main.a"
	"%UCRT64_LIB%\libopenmpt.dll.a"
	"%UCRT64_LIB%\libvorbis.dll.a"
	"%UCRT64_LIB%\libvorbisfile.dll.a"
	"%UCRT64_LIB%\libogg.dll.a"
	"%UCRT64_LIB%\libmpg123.dll.a"
	"%UCRT64_LIB%\libz.a"
	"%UCRT64_LIB%\libpng.a"
	"%UCRT64_LIB%\libjpeg.a"
	"%UCRT64_INC%\SDL2\SDL.h"
	"%UCRT64_INC%\libopenmpt\libopenmpt.h"
	"%UCRT64_INC%\vorbis\vorbisfile.h"
	"%UCRT64_INC%\ogg\ogg.h"
	"%UCRT64_INC%\mpg123.h"
	"%UCRT64_INC%\zlib.h"
	"%UCRT64_INC%\libpng16\png.h"
	"%UCRT64_INC%\jpeglib.h"
) do (
	if not exist %%~F set "MISSING=!MISSING! %%~nxF"
)
if defined MISSING goto err_no_deps

where python >nul 2>nul
if errorlevel 1 goto err_no_python

REM --- Reject a build directory configured by another generator or toolchain -
set "CACHED_GEN="
set "CACHED_CC="
if not exist "%BUILD_DIR%\CMakeCache.txt" goto cache_ok
for /f "tokens=1,* delims==" %%A in ('findstr /b /c:"CMAKE_GENERATOR:INTERNAL=" "%BUILD_DIR%\CMakeCache.txt"') do set "CACHED_GEN=%%B"
for /f "tokens=1,* delims==" %%A in ('findstr /b /c:"CMAKE_C_COMPILER:FILEPATH=" "%BUILD_DIR%\CMakeCache.txt"') do set "CACHED_CC=%%B"
if /i not "!CACHED_GEN!"=="%CMAKE_GENERATOR%" (
	echo Error: "%BUILD_DIR%" is configured for generator "!CACHED_GEN!", not "%CMAKE_GENERATOR%".
	echo        Run "build.bat clean" to reconfigure from scratch.
	goto err_pause
)
REM The compiler path may be recorded with either separator, so normalize both
REM sides of the comparison to forward slashes before checking the toolchain.
set "CACHED_CC_NORM=!CACHED_CC:\=/!"
set "UCRT64_BIN_NORM=%UCRT64_BIN:\=/%"
if /i not "!CACHED_CC_NORM!"=="!UCRT64_BIN_NORM!/cc.exe" if /i not "!CACHED_CC_NORM!"=="!UCRT64_BIN_NORM!/gcc.exe" (
	echo Error: "%BUILD_DIR%" is configured for compiler "!CACHED_CC!".
	echo        Run "build.bat clean" to reconfigure with the UCRT64 toolchain.
	goto err_pause
)
:cache_ok

REM --- Clean -----------------------------------------------------------------
if defined DO_CLEAN (
	if /i "%BUILD_DIR%"=="build" goto clean_ok
	if /i "%BUILD_DIR%"=="build-debug" goto clean_ok
	echo Error: refusing to delete "%BUILD_DIR%"; only "build" and "build-*" are allowed.
	goto err_pause
)
:clean_ok
if not defined DO_CLEAN goto clean_done
if not exist "%BUILD_DIR%" goto clean_done
echo Removing "%BUILD_DIR%" ...
rmdir /s /q "%BUILD_DIR%"
if exist "%BUILD_DIR%" (
	echo Error: could not remove "%BUILD_DIR%". Close programs using it and retry.
	goto err_pause
)
echo Cleaned "%BUILD_DIR%".
echo.
:clean_done

REM --- Configure -------------------------------------------------------------
echo [1/3] Configuring with CMake ...
set "CMAKE_OPTS=-DCMAKE_BUILD_TYPE=%BUILD_TYPE% -DBUILD_STATIC=%BUILD_STATIC% -DCONSOLE=%CONSOLE%"
"%CMAKE_EXE%" -S . -B "%BUILD_DIR%" -G "%CMAKE_GENERATOR%" %CMAKE_OPTS%
if errorlevel 1 (
	echo.
	echo Error: CMake configuration failed.
	goto err_pause
)

REM --- Build ------------------------------------------------------------------
echo.
echo [2/3] Building ...
set /a "__T0=%TIME:~0,2%*3600 + 1%TIME:~3,2% - 100*3600 + 1%TIME:~6,2% - 100*60"
"%CMAKE_EXE%" --build "%BUILD_DIR%" --parallel
set "__RC=%ERRORLEVEL%"
set /a "__T1=%TIME:~0,2%*3600 + 1%TIME:~3,2% - 100*3600 + 1%TIME:~6,2% - 100*60"
set /a "__ELAPSED=__T1 - __T0"
if %__ELAPSED% LSS 0 set /a "__ELAPSED=__ELAPSED + 86400"
if not "%__RC%"=="0" (
	echo.
	echo Error: build failed with exit code %__RC%.
	goto err_pause
)

REM --- Strip and verify ------------------------------------------------------
echo.
echo [3/3] Verifying output ...
if /i "%BUILD_STATIC%"=="ON" if /i "%CONSOLE%"=="OFF" (
	"%CMAKE_STRIP%" -s "%BUILD_DIR%\pvz-portable.exe"
	if errorlevel 1 echo Warning: could not strip "%BUILD_DIR%\pvz-portable.exe".
)

set "EXE_PATH="
if exist "%BUILD_DIR%\pvz-portable.exe" set "EXE_PATH=%BUILD_DIR%\pvz-portable.exe"
if exist "dist\pvz-portable.exe" set "EXE_PATH=dist\pvz-portable.exe"
if not defined EXE_PATH (
	echo Error: the build finished but "%BUILD_DIR%\pvz-portable.exe" is missing.
	goto err_pause
)
for %%F in ("%EXE_PATH%") do echo   Executable : %%~fF  ^(%%~zF bytes^)
if not exist "dist\pvz-portable.exe" goto final_report
if exist "dist\main.pak" (
	for %%F in ("dist\main.pak") do echo   Resources  : "%%~fF"  ^(%%~zF bytes^)
) else (
	echo   Resources  : "dist\main.pak" is missing ^(check res\main^).
)
if exist "dist\properties" (
	echo   Properties : "dist\properties" found.
) else (
	echo   Properties : "dist\properties" is missing ^(check res\properties^).
)

:final_report
echo.
echo Build succeeded in ~%__ELAPSED%s.
if /i "%BUILD_STATIC%"=="ON" echo Run it with: run-pvz.bat
if /i not "%BUILD_STATIC%"=="ON" echo Run it with: %EXE_PATH%
if not defined PVZ_NO_PAUSE pause
popd
endlocal
exit /b 0

REM --- Error exits ------------------------------------------------------------
:err_pause
if not defined PVZ_NO_PAUSE pause
popd
endlocal
exit /b 1

:err_no_msys2
echo Error: MSYS2 UCRT64 was not found ^(looked at "%MSYS2_ROOT_MSG%"^).
echo.
echo   Install MSYS2 from https://www.msys2.org/, then in the "MSYS2 UCRT64" shell run:
echo     pacman -S --needed %PACMAN_PACKAGES%
echo.
echo   If MSYS2 lives somewhere else, point the script at it first:
echo     set MSYS2_ROOT=D:\msys64
goto err_pause

:err_no_tools
echo Error: required tools are missing from "%MSYS2_ROOT%":!MISSING!
echo.
echo   In the "MSYS2 UCRT64" shell run:
echo     pacman -S --needed %PACMAN_PACKAGES%
goto err_pause

:err_no_deps
echo Error: development packages are missing from "%MSYS2_ROOT%":!MISSING!
echo.
echo   In the "MSYS2 UCRT64" shell run:
echo     pacman -S --needed mingw-w64-ucrt-x86_64-libjpeg-turbo mingw-w64-ucrt-x86_64-libopenmpt mingw-w64-ucrt-x86_64-libogg mingw-w64-ucrt-x86_64-libpng mingw-w64-ucrt-x86_64-libvorbis mingw-w64-ucrt-x86_64-mpg123 mingw-w64-ucrt-x86_64-SDL2
goto err_pause

:err_no_python
echo Error: Python 3.9+ was not found. CMake needs it to pack "main.pak".
echo.
echo   Install Python, or add it to MSYS2 with:
echo     pacman -S mingw-w64-ucrt-x86_64-python
goto err_pause
