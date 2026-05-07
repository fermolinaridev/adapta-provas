@echo off
:: =============================================================================
::  AdaptaProvas -- Abre o front-end no navegador
::  Detecta Chrome / Edge / Firefox / Brave instalados e abre direto neles,
::  ignorando associacoes do Windows que podem apontar pro VS Code.
:: =============================================================================

cd /d "%~dp0"

powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0abrir-browser.ps1"

if %ERRORLEVEL% neq 0 (
    echo.
    echo  Nao foi possivel abrir o navegador automaticamente.
    echo  Abra manualmente: %~dp0web\index.html
    echo.
    pause
)

exit /b %ERRORLEVEL%
