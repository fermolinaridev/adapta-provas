@echo off
:: =============================================================================
::  AdaptaProvas -- Versao linha de comando (CLI)
::  Instala gcc se necessario, compila e abre no terminal.
::  Para a versao web (navegador), use: start.bat
:: =============================================================================

title AdaptaProvas CLI

cd /d "%~dp0"

where powershell >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo.
    echo  [ERRO] PowerShell nao encontrado.
    pause
    exit /b 1
)

powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0setup-cli.ps1"

set PS_EXIT=%ERRORLEVEL%
if %PS_EXIT% neq 0 (
    echo.
    echo  O script terminou com codigo %PS_EXIT%.
    pause
)

exit /b %PS_EXIT%
