# =============================================================================
#  AdaptaProvas -- Setup e Launcher automatico (Windows)
#  Uso: executar via start.bat OU diretamente no PowerShell
#  O script:
#    1) Verifica se o gcc esta instalado
#    2) Se nao estiver, instala o MinGW-W64 via winget automaticamente
#    3) Atualiza o PATH da sessao atual
#    4) Compila o projeto (so se o .exe nao existir ou os fontes forem mais novos)
#    5) Executa o aplicativo
# =============================================================================

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Titulo($msg) {
    Write-Host ""
    Write-Host "  $msg" -ForegroundColor Cyan
    Write-Host ("  " + ("-" * ($msg.Length + 1))) -ForegroundColor DarkGray
}

function Ok($msg)    { Write-Host "  [OK]  $msg" -ForegroundColor Green  }
function Info($msg)  { Write-Host "  [..]  $msg" -ForegroundColor White  }
function Aviso($msg) { Write-Host "  [!!]  $msg" -ForegroundColor Yellow }
function Erro($msg)  { Write-Host "  [ERR] $msg" -ForegroundColor Red    }

Clear-Host
Write-Host ""
Write-Host "  ======================================================================" -ForegroundColor Magenta
Write-Host "         AdaptaProvas  --  Instalacao e Inicializacao                   " -ForegroundColor Magenta
Write-Host "  ======================================================================" -ForegroundColor Magenta
Write-Host ""

# ---------------------------------------------------------------------------
#  Diretorio do projeto
# ---------------------------------------------------------------------------

$ProjetoDiretorio = $PSScriptRoot
if (-not $ProjetoDiretorio) {
    $ProjetoDiretorio = Split-Path -Parent $MyInvocation.MyCommand.Path
}
if (-not $ProjetoDiretorio -or $ProjetoDiretorio -eq '') {
    $ProjetoDiretorio = (Get-Location).Path
}
Set-Location $ProjetoDiretorio
Info "Diretorio do projeto: $ProjetoDiretorio"

# ---------------------------------------------------------------------------
#  ETAPA 1: verificar / instalar gcc (MinGW-W64)
# ---------------------------------------------------------------------------

Titulo "Etapa 1/3 -- Verificando compilador C (gcc)"

function Atualizar-Path {
    $mPath = [System.Environment]::GetEnvironmentVariable('Path', 'Machine')
    $uPath = [System.Environment]::GetEnvironmentVariable('Path', 'User')
    $partes = @($mPath, $uPath) | Where-Object { $_ -ne $null -and $_ -ne '' }
    $env:Path = $partes -join ';'
}

function Localizar-Gcc {
    if (Get-Command gcc -ErrorAction SilentlyContinue) {
        return (Get-Command gcc -ErrorAction SilentlyContinue).Source
    }
    $candidatos = @(
        "$env:LOCALAPPDATA\Microsoft\WinGet\Packages\BrechtSanders.WinLibs*\mingw64\bin\gcc.exe",
        "$env:LOCALAPPDATA\Microsoft\WinGet\Packages\BrechtSanders.WinLibs*\bin\gcc.exe",
        "C:\mingw64\bin\gcc.exe",
        "C:\msys64\ucrt64\bin\gcc.exe",
        "C:\msys64\mingw64\bin\gcc.exe"
    )
    foreach ($glob in $candidatos) {
        try {
            $encontrado = Get-ChildItem -Path $glob -ErrorAction SilentlyContinue | Select-Object -First 1
            if ($encontrado) { return $encontrado.FullName }
        } catch {}
    }
    return $null
}

$gccPath = Localizar-Gcc

if ($gccPath) {
    $gccDir = Split-Path -Parent $gccPath
    if ($env:Path -notlike "*$gccDir*") {
        $env:Path = "$gccDir;$env:Path"
    }
    $gccVer = (& gcc --version 2>&1) | Select-Object -First 1
    Ok "gcc encontrado: $gccVer"
} else {
    Aviso "gcc nao encontrado. Instalando MinGW-W64 via winget..."
    Write-Host ""

    if (-not (Get-Command winget -ErrorAction SilentlyContinue)) {
        Erro "winget nao esta disponivel neste sistema."
        Write-Host ""
        Write-Host "  Instale o gcc manualmente:" -ForegroundColor Yellow
        Write-Host "    https://github.com/brechtsanders/winlibs_mingw/releases" -ForegroundColor White
        Write-Host "  Depois adicione a pasta bin\ do gcc ao PATH e rode start.bat novamente." -ForegroundColor White
        Write-Host ""
        Read-Host "  Pressione ENTER para sair"
        exit 1
    }

    try {
        Info "Executando winget install BrechtSanders.WinLibs.POSIX.UCRT ..."
        Write-Host ""
        winget install `
            --id BrechtSanders.WinLibs.POSIX.UCRT `
            -e `
            --accept-package-agreements `
            --accept-source-agreements
        Write-Host ""
    } catch {
        Erro "Falha no winget: $_"
        Write-Host ""
        Write-Host "  Instale manualmente: https://github.com/brechtsanders/winlibs_mingw/releases" -ForegroundColor Yellow
        Read-Host "  Pressione ENTER para sair"
        exit 1
    }

    Atualizar-Path
    $gccPath = Localizar-Gcc

    if (-not $gccPath) {
        Erro "gcc ainda nao foi encontrado apos a instalacao."
        Aviso "Feche este terminal, abra um novo e execute start.bat novamente."
        Write-Host ""
        Read-Host "  Pressione ENTER para sair"
        exit 1
    }

    $gccDir = Split-Path -Parent $gccPath
    if ($env:Path -notlike "*$gccDir*") {
        $env:Path = "$gccDir;$env:Path"
    }
    $gccVer = (& gcc --version 2>&1) | Select-Object -First 1
    Ok "gcc instalado com sucesso: $gccVer"
}

# ---------------------------------------------------------------------------
#  ETAPA 2: compilar (so se necessario)
# ---------------------------------------------------------------------------

Titulo "Etapa 2/3 -- Compilando o projeto"

$EXE       = Join-Path $ProjetoDiretorio "adapta_provas.exe"
$BUILD_DIR = Join-Path $ProjetoDiretorio "build"
$INC_DIR   = Join-Path $ProjetoDiretorio "include"
$SRC_DIR   = Join-Path $ProjetoDiretorio "src"

$SRCS = @("main.c","prova.c","perfis.c","utils.c") | ForEach-Object {
    Join-Path $SRC_DIR $_
}

$precisaCompilar = $false

if (-not (Test-Path $EXE)) {
    Info "Binario nao encontrado -- compilando pela primeira vez..."
    $precisaCompilar = $true
} else {
    $exeTime = (Get-Item $EXE).LastWriteTime
    $fontesNovos = Get-ChildItem -Path $SRC_DIR, $INC_DIR -Include "*.c","*.h" -Recurse |
                   Where-Object { $_.LastWriteTime -gt $exeTime }
    if ($fontesNovos) {
        $nomes = ($fontesNovos | ForEach-Object { $_.Name }) -join ', '
        Info "Fontes modificados ($nomes) -- recompilando..."
        $precisaCompilar = $true
    } else {
        Ok "Binario ja esta atualizado. Pulando compilacao."
    }
}

if ($precisaCompilar) {
    if (-not (Test-Path $BUILD_DIR)) {
        New-Item -ItemType Directory -Path $BUILD_DIR | Out-Null
    }

    $objs = @()
    foreach ($src in $SRCS) {
        $base = [System.IO.Path]::GetFileNameWithoutExtension($src)
        $obj  = Join-Path $BUILD_DIR "$base.o"
        Info "Compilando $([System.IO.Path]::GetFileName($src)) ..."
        & gcc -std=c99 -Wall -Wextra -O2 -I"$INC_DIR" -c "$src" -o "$obj"
        if ($LASTEXITCODE -ne 0) {
            Erro "Falha ao compilar $src (codigo $LASTEXITCODE)"
            Read-Host "`n  Pressione ENTER para sair"
            exit 1
        }
        $objs += $obj
    }

    Info "Linkando ..."
    & gcc -std=c99 -O2 -o "$EXE" $objs
    if ($LASTEXITCODE -ne 0) {
        Erro "Falha na linkagem (codigo $LASTEXITCODE)"
        Read-Host "`n  Pressione ENTER para sair"
        exit 1
    }

    Ok "Compilacao concluida: adapta_provas.exe"
}

# ---------------------------------------------------------------------------
#  ETAPA 3: executar
# ---------------------------------------------------------------------------

Titulo "Etapa 3/3 -- Iniciando AdaptaProvas"
Write-Host ""
Info "Pressione Ctrl+C a qualquer momento para sair."
Write-Host ""
Start-Sleep -Milliseconds 400

& "$EXE"

Write-Host ""
Write-Host "  Sessao encerrada." -ForegroundColor DarkGray

# Pausa automatica so quando aberto por duplo clique (fora de terminal integrado)
try {
    $parentName = (Get-Process -Id (Get-Process -Id $PID).Parent.Id -ErrorAction SilentlyContinue).Name
    $terminaisConhecidos = @('WindowsTerminal','wt','pwsh','powershell','Code','devenv')
    if ($parentName -notin $terminaisConhecidos) {
        Read-Host "`n  Pressione ENTER para fechar"
    }
} catch {}
