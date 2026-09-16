@echo on
REM Launcher for PvZ-Portable (Windows)
REM Usage: run-pvz.bat [extra args]

setlocal

pushd "%~dp0dist" || (
	echo Error: dist\ directory not found in "%~dp0"
	pause
	exit /b 1
)

"%~dp0dist\pvz-portable.exe" %*

popd
endlocal

