@echo off
:: Para o servidor HTTP local do AdaptaProvas (porta 8765)
title Parar servidor AdaptaProvas

echo.
echo  Parando servidor local na porta 8765...
echo.

powershell -NoProfile -Command ^
  "Get-NetTCPConnection -LocalPort 8765 -ErrorAction SilentlyContinue | ForEach-Object { Stop-Process -Id $_.OwningProcess -Force -ErrorAction SilentlyContinue }; Get-Job | Where-Object { $_.Command -like '*HttpListener*' } | Stop-Job -ErrorAction SilentlyContinue; Get-Job | Where-Object { $_.Command -like '*HttpListener*' } | Remove-Job -Force -ErrorAction SilentlyContinue; Write-Host '  Servidor parado.' -ForegroundColor Green"

echo.
timeout /t 2 /nobreak >nul
exit /b 0
