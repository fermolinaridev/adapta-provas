@echo off
:: Cria atalho do AdaptaProvas na area de trabalho do Windows
title AdaptaProvas - Criar atalho

cd /d "%~dp0"

powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0criar-atalho.ps1"

if %ERRORLEVEL% neq 0 (
    echo.
    pause
)

exit /b %ERRORLEVEL%
