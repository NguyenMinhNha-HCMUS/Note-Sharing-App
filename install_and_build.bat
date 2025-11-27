@echo off
echo ==========================================
echo      SECURE NOTE APP - AUTO INSTALL
echo ==========================================
echo.

echo [1/2] Setting up dependencies...
powershell -ExecutionPolicy Bypass -File "scripts/setup_dependencies.ps1"
if %errorlevel% neq 0 (
    echo.
    echo [ERROR] Failed to setup dependencies.
    pause
    exit /b %errorlevel%
)

echo.
echo [2/2] Building applications...
powershell -ExecutionPolicy Bypass -File "scripts/build.ps1"
if %errorlevel% neq 0 (
    echo.
    echo [ERROR] Build failed.
    echo Please make sure you have g++ (MinGW) and OpenSSL installed.
    pause
    exit /b %errorlevel%
)

echo.
echo ==========================================
echo      INSTALLATION COMPLETE!
echo ==========================================
echo.
echo You can now run:
echo   - server_app.exe
echo   - client_app.exe
echo.
pause
