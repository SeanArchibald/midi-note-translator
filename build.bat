@echo off
setlocal EnableDelayedExpansion

:: Build script for Windows

echo Building Scale Scaler CLAP Plugin for Windows...
echo ================================================

set "SCRIPT_DIR=%~dp0"
set "BUILD_DIR=%SCRIPT_DIR%build"
set "INSTALL_PREFIX=%SCRIPT_DIR%install"

:: Check for Visual Studio
where cl >nul 2>nul
if %errorlevel% neq 0 (
    echo Error: Visual Studio compiler not found in PATH.
    echo Please run this script from a Visual Studio Developer Command Prompt.
    exit /b 1
)

:: Clean previous build
if exist "%BUILD_DIR%" (
    echo Cleaning previous build...
    rmdir /s /q "%BUILD_DIR%"
)
mkdir "%BUILD_DIR%"

:: Configure
echo Configuring with CMake...
cd "%BUILD_DIR%"

cmake .. -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="%INSTALL_PREFIX%"
if %errorlevel% neq 0 (
    echo CMake configuration failed.
    exit /b 1
)

:: Build
echo Building...
nmake
if %errorlevel% neq 0 (
    echo Build failed.
    exit /b 1
)

:: Install (local copy)
echo Installing locally...
nmake install
if %errorlevel% neq 0 (
    echo Install failed.
    exit /b 1
)

echo.
echo ================================================
echo Build complete!
echo.
echo Plugin location:
echo   %INSTALL_PREFIX%\scale-scaler.clap
echo.
echo To install system-wide, copy to:
echo   %%LOCALAPPDATA%%\Programs\Common\CLAP\
echo.
echo Or run: cmake --install %BUILD_DIR%
echo ================================================

cd "%SCRIPT_DIR%"
endlocal