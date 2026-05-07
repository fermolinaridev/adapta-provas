@echo off
:: =============================================================================
::  AdaptaProvas -- Lancador inteligente
::  Verifica requisitos, instala o que faltar, e abre o front-end no navegador.
::  Para a versao terminal (CLI), execute: start-cli.bat
:: =============================================================================

title AdaptaProvas

cd /d "%~dp0"

where powershell >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo.
    echo  [ERRO] PowerShell nao encontrado neste sistema.
    echo  Abra manualmente: %~dp0web\index.html
    pause
    exit /b 1
)

powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0setup.ps1"

set PS_EXIT=%ERRORLEVEL%
if %PS_EXIT% neq 0 (
    echo.
    pause
)

exit /b %PS_EXIT%
