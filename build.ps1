param(
    [string]$ButanoPath = 'D:/abc/gbc/butano/butano',
    [string]$DevkitProPath = 'D:/devkitPro',
    [int]$Jobs = 8
)
$ErrorActionPreference = 'Stop'
if (!(Test-Path -LiteralPath (Join-Path $ButanoPath 'butano.mak'))) { throw 'ButanoPath must point to the folder containing butano.mak.' }
$env:DEVKITPRO = $DevkitProPath.Replace('\','/')
$env:DEVKITARM = "$env:DEVKITPRO/devkitARM"
$env:PATH = "$DevkitProPath/devkitARM/bin;$DevkitProPath/tools/bin;$DevkitProPath/msys2/usr/bin;$env:PATH"
Push-Location $PSScriptRoot
try {
    & make "LIBBUTANO=$($ButanoPath.Replace('\','/'))" "-j$Jobs"
    if ($LASTEXITCODE -ne 0) { throw "Build failed: $LASTEXITCODE" }
    Write-Output "ROM: $PSScriptRoot/corpse_party_chapter1.gba"
} finally { Pop-Location }
