# =============================================================================
#  AdaptaProvas -- Cria atalho na area de trabalho
#  Aponta para start.bat (que sobe o servidor e abre o navegador).
# =============================================================================

$ErrorActionPreference = 'Stop'

function Ok($msg)    { Write-Host "  [OK]  $msg" -ForegroundColor Green  }
function Info($msg)  { Write-Host "  [..]  $msg" -ForegroundColor White  }
function Aviso($msg) { Write-Host "  [!!]  $msg" -ForegroundColor Yellow }
function Erro($msg)  { Write-Host "  [ERR] $msg" -ForegroundColor Red    }

Write-Host ""
Write-Host "  ======================================================================" -ForegroundColor Magenta
Write-Host "         AdaptaProvas  --  Criando atalho na area de trabalho           " -ForegroundColor Magenta
Write-Host "  ======================================================================" -ForegroundColor Magenta
Write-Host ""

# --------------------------------------------------------------------------- #
#  Caminhos
# --------------------------------------------------------------------------- #

$ProjetoDir = $PSScriptRoot
if (-not $ProjetoDir) { $ProjetoDir = Split-Path -Parent $MyInvocation.MyCommand.Path }
if (-not $ProjetoDir) { $ProjetoDir = (Get-Location).Path }

$StartBat = Join-Path $ProjetoDir "start.bat"
if (-not (Test-Path $StartBat)) {
    Erro "start.bat nao encontrado em: $StartBat"
    Read-Host "  Pressione ENTER para sair"
    exit 1
}

# Detecta a area de trabalho do usuario (em PT-BR pode ser "Desktop" ou "Area de Trabalho")
$Desktop = [Environment]::GetFolderPath("Desktop")
if (-not $Desktop -or -not (Test-Path $Desktop)) {
    Erro "Nao foi possivel localizar a area de trabalho."
    Read-Host "  Pressione ENTER para sair"
    exit 1
}

Info "Projeto:       $ProjetoDir"
Info "Area trabalho: $Desktop"

# --------------------------------------------------------------------------- #
#  Criar atalho via COM (WScript.Shell)
# --------------------------------------------------------------------------- #

$NomeAtalho = "AdaptaProvas.lnk"
$CaminhoAtalho = Join-Path $Desktop $NomeAtalho

Write-Host ""
Info "Criando atalho: $CaminhoAtalho"

try {
    $shell    = New-Object -ComObject WScript.Shell
    $atalho   = $shell.CreateShortcut($CaminhoAtalho)

    $atalho.TargetPath       = $StartBat
    $atalho.WorkingDirectory = $ProjetoDir
    $atalho.Description      = "AdaptaProvas - Adaptador de Provas Acessivel"
    $atalho.WindowStyle      = 7   # 7 = minimizado (evita janela preta piscando)

    # Tenta usar um icone bonito do Windows.
    # imageres.dll tem varios icones; 109 = boneco com cap. de formatura,
    # 27 = pasta de documentos colorida, 200 = engrenagem.
    $iconePadrao = "$env:SystemRoot\System32\imageres.dll,109"
    $atalho.IconLocation = $iconePadrao

    $atalho.Save()

    # Liberar COM
    [System.Runtime.InteropServices.Marshal]::ReleaseComObject($shell) | Out-Null

    if (Test-Path $CaminhoAtalho) {
        Write-Host ""
        Ok "Atalho criado com sucesso!"
        Write-Host ""
        Write-Host "  Caminho: $CaminhoAtalho" -ForegroundColor DarkGray
        Write-Host "  Aponta para: $StartBat" -ForegroundColor DarkGray
        Write-Host ""
        Write-Host "  Voce ja pode dar duplo clique no atalho da area de trabalho!" -ForegroundColor Cyan
        Write-Host ""
    } else {
        Erro "Atalho nao foi criado (motivo desconhecido)."
        exit 1
    }

} catch {
    Erro "Falha ao criar atalho: $_"
    Read-Host "  Pressione ENTER para sair"
    exit 1
}

Start-Sleep -Seconds 2
