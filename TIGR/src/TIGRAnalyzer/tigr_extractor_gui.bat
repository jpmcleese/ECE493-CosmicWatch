@echo off
:: TIGR SD Card Data Extractor - GUI Launcher with Auto-Elevation

:: Check for admin rights
net session >nul 2>&1
if %errorLevel% == 0 (
    goto :run
) else (
    goto :elevate
)

:elevate
:: Request admin elevation
powershell -Command "Start-Process '%~f0' -Verb RunAs"
exit /b

:run
:: Change to script directory
cd /d "%~dp0"

:: Run the GUI Python script
pythonw tigr_extractor_gui.py

exit