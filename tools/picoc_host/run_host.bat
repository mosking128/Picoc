@echo off
setlocal

cd /d "%~dp0"

python src\app.py
if errorlevel 1 (
    echo.
    echo Failed to start PicoC Host Tool.
    echo Please install dependencies manually first:
    echo   pip install -r src\requirements.txt
    echo.
    echo If your environment uses proxy or mirror settings, clear them first:
    echo   set HTTP_PROXY=
    echo   set HTTPS_PROXY=
    echo.
    pause
    exit /b 1
)

endlocal
