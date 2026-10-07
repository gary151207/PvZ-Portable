@echo off
REM Launcher for PvZ-Portable-Gaga (Windows)
REM Usage: run-pvz.bat [extra args]
REM
REM Runs dist\pvz-portable.exe, which loads its game data (main.pak and
REM properties\) from that same directory. Build it first with build.bat.
REM
REM Examples:
REM   run-pvz.bat                        normal 4x zombie spawning
REM   run-pvz.bat -zombiemult=1          original spawning
REM   run-pvz.bat -zombiemult=8          eight times the zombies

setlocal

if not exist "%~dp0dist\pvz-portable.exe" (
	echo Error: "%~dp0dist\pvz-portable.exe" not found.
	echo        Build it first by running build.bat
	pause
	exit /b 1
)

pushd "%~dp0dist" || (
	echo Error: cannot enter "%~dp0dist".
	pause
	exit /b 1
)

"%~dp0dist\pvz-portable.exe" %*

popd
endlocal
