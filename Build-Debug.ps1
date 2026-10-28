$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

$root = $PSScriptRoot
$buildDir = Join-Path $root "Build\windows-vs2022-x64"
$gameDataDir = Join-Path $root "GameData"
$debugExe = Join-Path $gameDataDir "PhoenixClient_Debug.exe"

function Invoke-Checked {
    param(
        [Parameter(Mandatory = $true)]
        [string] $Command,
        [Parameter(ValueFromRemainingArguments = $true)]
        [string[]] $Arguments
    )

    & $Command @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "$Command failed with exit code $LASTEXITCODE"
    }
}

$runningClient = Get-Process -Name "PhoenixClient*" -ErrorAction SilentlyContinue
if ($runningClient) {
    $ids = ($runningClient | ForEach-Object { "$($_.ProcessName):$($_.Id)" }) -join ", "
    throw "PhoenixClient is running and may lock the output exe. Close it first. Running: $ids"
}

Push-Location $root
try {
    if (-not (Test-Path (Join-Path $buildDir "CMakeCache.txt"))) {
        Invoke-Checked cmake --preset windows-vs2022-x64
    }

    Invoke-Checked cmake --build --preset windows-vs2022-x64-debug --target PhoenixClient --clean-first

    if (-not (Test-Path $debugExe)) {
        throw "Debug output was not produced: $debugExe"
    }

    $debugFile = Get-Item $debugExe
    Write-Host "Debug build complete:"
    Write-Host "  $($debugFile.FullName)"
    Write-Host "  $($debugFile.Length) bytes"
}
finally {
    Pop-Location
}
