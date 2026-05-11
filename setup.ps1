# =============================================================================
#  AdaptaProvas -- Verificacao de requisitos + lancamento do front-end
#  Uso: chamado pelo start.bat ao duplo clique.
#  O script:
#    1) Verifica se os arquivos web/ existem
#    2) Verifica se ha um navegador instalado (Chrome/Edge/Brave/Firefox/...)
#    3) Se nao houver, instala o Microsoft Edge via winget
#    4) Opcionalmente verifica se gcc esta instalado (para a CLI)
#    5) Abre o front-end no navegador detectado
# =============================================================================

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

# --------------------------------------------------------------------------- #
#  Utilitarios de exibicao
# --------------------------------------------------------------------------- #

function Titulo($msg) {
    Write-Host ""
    Write-Host "  $msg" -ForegroundColor Cyan
    Write-Host ("  " + ("-" * ($msg.Length + 1))) -ForegroundColor DarkGray
}
function Ok($msg)    { Write-Host "  [OK]  $msg" -ForegroundColor Green  }
function Info($msg)  { Write-Host "  [..]  $msg" -ForegroundColor White  }
function Aviso($msg) { Write-Host "  [!!]  $msg" -ForegroundColor Yellow }
function Erro($msg)  { Write-Host "  [ERR] $msg" -ForegroundColor Red    }

# --------------------------------------------------------------------------- #
#  Banner
# --------------------------------------------------------------------------- #

Clear-Host
Write-Host ""
Write-Host "  ======================================================================" -ForegroundColor Magenta
Write-Host "         AdaptaProvas  --  Verificacao do Sistema e Inicializacao       " -ForegroundColor Magenta
Write-Host "  ======================================================================" -ForegroundColor Magenta
Write-Host ""

$ProjetoDir = $PSScriptRoot
if (-not $ProjetoDir) { $ProjetoDir = Split-Path -Parent $MyInvocation.MyCommand.Path }
if (-not $ProjetoDir) { $ProjetoDir = (Get-Location).Path }
Set-Location $ProjetoDir
Info "Diretorio do projeto: $ProjetoDir"

# --------------------------------------------------------------------------- #
#  ETAPA 1: verificar arquivos do front-end
# --------------------------------------------------------------------------- #

Titulo "Etapa 1/4 -- Verificando arquivos do front-end"

$IndexPath = Join-Path $ProjetoDir "web\index.html"
$ArquivosNecessarios = @(
    "web\index.html",
    "web\styles.css",
    "web\js\perfis.js",
    "web\js\adaptador.js",
    "web\js\parser.js",
    "web\js\ia.js",
    "web\js\exemplos.js",
    "web\js\app.js"
)

$faltando = @()
foreach ($arq in $ArquivosNecessarios) {
    $caminho = Join-Path $ProjetoDir $arq
    if (-not (Test-Path $caminho)) { $faltando += $arq }
}

if ($faltando.Count -gt 0) {
    Erro "Arquivos do front-end ausentes:"
    foreach ($f in $faltando) { Write-Host "         - $f" -ForegroundColor Red }
    Write-Host ""
    Aviso "Reclone o repositorio: git clone https://github.com/fernandomunhozmolinari/adapta-provas"
    Read-Host "  Pressione ENTER para sair"
    exit 1
}
Ok "Todos os $($ArquivosNecessarios.Count) arquivos do front-end estao presentes."

# --------------------------------------------------------------------------- #
#  ETAPA 2: detectar navegador instalado
# --------------------------------------------------------------------------- #

Titulo "Etapa 2/4 -- Detectando navegador"

$Browsers = @(
    @{ Nome="Google Chrome";    Path="$env:ProgramFiles\Google\Chrome\Application\chrome.exe" },
    @{ Nome="Google Chrome";    Path="${env:ProgramFiles(x86)}\Google\Chrome\Application\chrome.exe" },
    @{ Nome="Google Chrome";    Path="$env:LOCALAPPDATA\Google\Chrome\Application\chrome.exe" },
    @{ Nome="Microsoft Edge";   Path="$env:ProgramFiles\Microsoft\Edge\Application\msedge.exe" },
    @{ Nome="Microsoft Edge";   Path="${env:ProgramFiles(x86)}\Microsoft\Edge\Application\msedge.exe" },
    @{ Nome="Brave";            Path="$env:ProgramFiles\BraveSoftware\Brave-Browser\Application\brave.exe" },
    @{ Nome="Brave";            Path="${env:ProgramFiles(x86)}\BraveSoftware\Brave-Browser\Application\brave.exe" },
    @{ Nome="Brave";            Path="$env:LOCALAPPDATA\BraveSoftware\Brave-Browser\Application\brave.exe" },
    @{ Nome="Mozilla Firefox";  Path="$env:ProgramFiles\Mozilla Firefox\firefox.exe" },
    @{ Nome="Mozilla Firefox";  Path="${env:ProgramFiles(x86)}\Mozilla Firefox\firefox.exe" },
    @{ Nome="Opera";            Path="$env:ProgramFiles\Opera\opera.exe" },
    @{ Nome="Opera";            Path="${env:ProgramFiles(x86)}\Opera\opera.exe" },
    @{ Nome="Opera";            Path="$env:LOCALAPPDATA\Programs\Opera\opera.exe" },
    @{ Nome="Vivaldi";          Path="$env:LOCALAPPDATA\Vivaldi\Application\vivaldi.exe" }
)

$BrowserEncontrado = $null
foreach ($b in $Browsers) {
    if (Test-Path $b.Path) {
        $BrowserEncontrado = $b
        break
    }
}

if ($BrowserEncontrado) {
    Ok "Navegador detectado: $($BrowserEncontrado.Nome)"
} else {
    Aviso "Nenhum navegador encontrado nos caminhos padrao."
    Write-Host ""

    if (Get-Command winget -ErrorAction SilentlyContinue) {
        Info "Instalando Microsoft Edge via winget..."
        Write-Host ""
        try {
            winget install --id Microsoft.Edge -e `
                --accept-package-agreements `
                --accept-source-agreements
            Write-Host ""
            # Tenta detectar de novo
            foreach ($b in $Browsers) {
                if (Test-Path $b.Path) { $BrowserEncontrado = $b; break }
            }
            if ($BrowserEncontrado) {
                Ok "Edge instalado: $($BrowserEncontrado.Nome)"
            } else {
                Erro "Instalacao concluida mas o executavel nao foi encontrado."
                Aviso "Reinicie o Windows e tente novamente."
                Read-Host "  Pressione ENTER para sair"
                exit 1
            }
        } catch {
            Erro "Falha no winget: $_"
            Read-Host "  Pressione ENTER para sair"
            exit 1
        }
    } else {
        Erro "winget nao esta disponivel."
        Aviso "Instale um navegador manualmente:"
        Write-Host "    Chrome: https://www.google.com/chrome/" -ForegroundColor White
        Write-Host "    Edge:   https://www.microsoft.com/edge/" -ForegroundColor White
        Read-Host "  Pressione ENTER para sair"
        exit 1
    }
}

# --------------------------------------------------------------------------- #
#  ETAPA 3: verificar gcc (opcional, so para CLI)
# --------------------------------------------------------------------------- #

Titulo "Etapa 3/4 -- Verificando suporte para versao CLI (opcional)"

$GccDisponivel = $false
if (Get-Command gcc -ErrorAction SilentlyContinue) {
    $gccVer = (& gcc --version 2>&1) | Select-Object -First 1
    Ok "gcc disponivel: $gccVer"
    $GccDisponivel = $true
} else {
    # Procura em locais conhecidos sem instalar
    $gccCandidatos = @(
        "$env:LOCALAPPDATA\Microsoft\WinGet\Packages\BrechtSanders.WinLibs*\mingw64\bin\gcc.exe",
        "C:\mingw64\bin\gcc.exe",
        "C:\msys64\ucrt64\bin\gcc.exe"
    )
    foreach ($glob in $gccCandidatos) {
        $g = Get-ChildItem -Path $glob -ErrorAction SilentlyContinue | Select-Object -First 1
        if ($g) {
            Ok "gcc encontrado em: $($g.FullName)"
            $GccDisponivel = $true
            break
        }
    }
    if (-not $GccDisponivel) {
        Info "gcc nao instalado (so necessario se voce quiser a CLI). Use start-cli.bat para instalar."
    }
}

# --------------------------------------------------------------------------- #
#  ETAPA 4: subir servidor HTTP local (resolve CORS) e abrir front-end
# --------------------------------------------------------------------------- #

Titulo "Etapa 4/4 -- Iniciando servidor local e abrindo AdaptaProvas"

$WebDir = Join-Path $ProjetoDir "web"
$Porta  = 8765
$Url    = "http://localhost:$Porta/index.html"

# Verifica se a porta ja esta em uso (instancia anterior do app)
$emUso = $false
try {
    $tcp = [System.Net.Sockets.TcpListener]::new([System.Net.IPAddress]::Loopback, $Porta)
    $tcp.Start()
    $tcp.Stop()
} catch {
    $emUso = $true
}

if ($emUso) {
    Info "Servidor ja esta rodando na porta $Porta. Reabrindo no navegador..."
} else {
    # Tenta iniciar via Python (script proprio com proxy para Pollinations)
    $servidorIniciado = $false
    $servidorPy = Join-Path $ProjetoDir "servidor.py"

    if ((Get-Command python -ErrorAction SilentlyContinue) -and (Test-Path $servidorPy)) {
        Info "Iniciando servidor Python (com proxy para Pollinations) na porta $Porta..."
        $procArgs = @{
            FilePath     = "python"
            ArgumentList = @("`"$servidorPy`"", "$Porta")
            WorkingDirectory = $ProjetoDir
            WindowStyle  = "Hidden"
            PassThru     = $true
        }
        try {
            $servidor = Start-Process @procArgs
            Start-Sleep -Milliseconds 1000
            $servidorIniciado = -not $servidor.HasExited
            if ($servidorIniciado) {
                Ok "Servidor Python com proxy ativo (IA Pollinations funcionando sem CORS)"
            }
        } catch {}
    } elseif (Get-Command python -ErrorAction SilentlyContinue) {
        # Fallback: python.exe sem o script proprio (sem proxy)
        Info "servidor.py nao encontrado. Usando servidor Python basico (sem proxy)..."
        $procArgs = @{
            FilePath     = "python"
            ArgumentList = @("-m", "http.server", "$Porta", "--bind", "127.0.0.1")
            WorkingDirectory = $WebDir
            WindowStyle  = "Hidden"
            PassThru     = $true
        }
        try {
            $servidor = Start-Process @procArgs
            Start-Sleep -Milliseconds 800
            $servidorIniciado = -not $servidor.HasExited
        } catch {}
        Aviso "Pollinations pode falhar. Use Gemini (gratuito) se a IA nao funcionar."
    }

    if (-not $servidorIniciado) {
        # Fallback: usa o HttpListener nativo do .NET via PowerShell em background job
        Info "Python nao encontrado. Usando servidor PowerShell nativo..."
        $servidor = Start-Job -ScriptBlock {
            param($pasta, $porta)
            Add-Type -AssemblyName System.Web
            $listener = [System.Net.HttpListener]::new()
            $listener.Prefixes.Add("http://127.0.0.1:$porta/")
            $listener.Start()
            try {
                while ($listener.IsListening) {
                    $ctx = $listener.GetContext()
                    $req = $ctx.Request
                    $res = $ctx.Response
                    $rel = [System.Web.HttpUtility]::UrlDecode($req.Url.LocalPath.TrimStart('/'))
                    if ([string]::IsNullOrEmpty($rel)) { $rel = "index.html" }
                    $arq = Join-Path $pasta $rel
                    if (Test-Path $arq -PathType Leaf) {
                        $bytes = [System.IO.File]::ReadAllBytes($arq)
                        $ext = [System.IO.Path]::GetExtension($arq).ToLower()
                        $mime = switch ($ext) {
                            ".html" { "text/html; charset=utf-8" }
                            ".css"  { "text/css; charset=utf-8" }
                            ".js"   { "application/javascript; charset=utf-8" }
                            ".json" { "application/json; charset=utf-8" }
                            ".svg"  { "image/svg+xml" }
                            default { "application/octet-stream" }
                        }
                        $res.ContentType = $mime
                        $res.ContentLength64 = $bytes.Length
                        $res.OutputStream.Write($bytes, 0, $bytes.Length)
                    } else {
                        $res.StatusCode = 404
                    }
                    $res.Close()
                }
            } finally {
                $listener.Stop()
            }
        } -ArgumentList $WebDir, $Porta
        Start-Sleep -Milliseconds 600
    }

    Ok "Servidor local rodando em http://localhost:$Porta"
}

Info "URL: $Url"
Info "Lancando $($BrowserEncontrado.Nome)..."

try {
    Start-Process -FilePath $BrowserEncontrado.Path -ArgumentList $Url
    Write-Host ""
    Ok "Pronto! O AdaptaProvas esta aberto no seu navegador."
    Write-Host ""
    Write-Host "  Dicas:" -ForegroundColor DarkGray
    Write-Host "    - Para a versao CLI no terminal: execute start-cli.bat" -ForegroundColor DarkGray
    Write-Host "    - Para parar o servidor: feche o navegador ou execute parar-servidor.bat" -ForegroundColor DarkGray
    Write-Host "    - O servidor evita problemas de CORS ao usar a IA" -ForegroundColor DarkGray
    Write-Host ""
    Start-Sleep -Seconds 2
    exit 0
} catch {
    Erro "Falha ao lancar o navegador: $_"
    Read-Host "  Pressione ENTER para sair"
    exit 1
}
