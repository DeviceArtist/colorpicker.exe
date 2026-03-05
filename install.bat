@echo off
echo Installing MinGW compiler environment...
echo Please ensure internet connectivity

REM Create temporary directory
set TEMP_DIR=\mingw_install

echo %TEMP_DIR%
mkdir "%TEMP_DIR%" 2>nul
cd /d "%TEMP_DIR%"

echo Downloading MinGW installer...
powershell -Command "Invoke-WebRequest -Uri 'https://github.com/brechtsanders/winlibs_mingw/releases/download/13.2.0-16.0.6-11.0.0-ucrt-r1/winlibs-x86_64-posix-seh-gcc-13.2.0-llvm-16.0.6-mingw-w64ucrt-11.0.0-r1.7z' -OutFile 'mingw.7z'"

echo Extracting files...
powershell -Command "Expand-Archive -Path 'mingw.7z' -DestinationPath 'C:\mingw64'"

echo Configuring environment variables...
setx PATH "%PATH%;C:\mingw64\bin"

@REM echo Cleaning temporary files...
@REM cd /d "%TEMP%"
@REM rmdir /s /q "%TEMP_DIR%"

echo.
echo MinGW installation completed!
echo Please restart command prompt to apply environment variables
echo Compiler installed to C:\mingw64\bin
echo Use gcc and g++ commands to compile C/C++ programs
pause