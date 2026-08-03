@echo off
chcp 65001 >nul
echo ==========================================
echo   ESP-IDF Build and Flash Script
echo ==========================================

set IDF_PATH=D:\Software\ESP-IDF
set IDF_TOOLS_PATH=D:\Software\Espressif
set PATH=D:\Software\Espressif\tools\cmake\3.30.2\bin;D:\Software\Espressif\tools\idf-git\2.44.0\cmd;D:\Software\Espressif\tools\xtensa-esp-elf\esp-14.2.0_20260121\xtensa-esp-elf\bin;D:\Software\Espressif\python_env\idf5.5_py3.11_env\Scripts;%PATH%

set IDF_COMPONENT_MANAGER_CACHE_DIR=%~dp0idf_cache
if not exist "%IDF_COMPONENT_MANAGER_CACHE_DIR%" mkdir "%IDF_COMPONENT_MANAGER_CACHE_DIR%"

set PYTHON=D:\Software\Espressif\python_env\idf5.5_py3.11_env\Scripts\python.exe

echo.
echo [1/4] Activating ESP-IDF...
call "%IDF_PATH%\export.bat"
if errorlevel 1 (
    echo ERROR: ESP-IDF activation failed!
    pause
    exit /b 1
)

echo.
echo [2/4] Setting target esp32s3...
cd /d "%~dp0ble_lvgl_project"
idf.py set-target esp32s3
if errorlevel 1 (
    echo ERROR: set-target failed!
    pause
    exit /b 1
)

echo.
echo [3/4] Building project...
idf.py build
if errorlevel 1 (
    echo ERROR: Build failed!
    pause
    exit /b 1
)

echo.
echo [4/4] Flashing to COM3...
idf.py -p COM3 flash monitor
pause
