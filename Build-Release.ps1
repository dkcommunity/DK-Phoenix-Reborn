param(
    [switch] $Clean,
    [switch] $NoPause
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

$root = $PSScriptRoot
$buildRoot = Join-Path $root "Build"
$buildDir = Join-Path $root "Build\windows-vs2022-x64"
$gameDataDir = Join-Path $root "GameData"
$releaseExe = Join-Path $gameDataDir "PhoenixClient.exe"

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

function Wait-BeforeExit {
    if (-not $NoPause -and [Environment]::UserInteractive) {
        Write-Host ""
        Read-Host "Press Enter to close"
    }
}

function Invoke-ReleaseBuild {
    $runningClient = Get-Process -Name "PhoenixClient*" -ErrorAction SilentlyContinue
    if ($runningClient) {
        $ids = ($runningClient | ForEach-Object { "$($_.ProcessName):$($_.Id)" }) -join ", "
        throw "PhoenixClient is running and may lock the output exe. Close it first. Running: $ids"
    }

    if ($Clean -and (Test-Path -LiteralPath $buildRoot)) {
        $resolvedRoot = (Resolve-Path -LiteralPath $root).Path
        $resolvedBuild = (Resolve-Path -LiteralPath $buildRoot).Path
        if (-not $resolvedBuild.StartsWith($resolvedRoot, [System.StringComparison]::OrdinalIgnoreCase)) {
            throw "Refusing to remove path outside project root: $resolvedBuild"
        }
        Remove-Item -LiteralPath $resolvedBuild -Recurse -Force
        Write-Host "Removed build directory: $resolvedBuild"
    }

    Push-Location $root
    try {
        if (-not (Test-Path (Join-Path $buildDir "CMakeCache.txt"))) {
            Invoke-Checked cmake --preset windows-vs2022-x64
        }

        Invoke-Checked cmake --build --preset windows-vs2022-x64-release --target PhoenixClient --clean-first

        if (-not (Test-Path $releaseExe)) {
            throw "Release output was not produced: $releaseExe"
        }

        $file = Get-Item $releaseExe
        Write-Host "Release build complete:"
        Write-Host "  $($file.FullName)"
        Write-Host "  $($file.Length) bytes"
        Write-Host "  Last write: $($file.LastWriteTime)"
    }
    finally {
        Pop-Location
    }
}

$exitCode = 0
try {
    Invoke-ReleaseBuild
}
catch {
    $exitCode = 1
    Write-Host ""
    Write-Host "Release build failed:" -ForegroundColor Red
    Write-Host $_.Exception.Message -ForegroundColor Red
    Write-Host ""
    Write-Host "Details:"
    $_ | Format-List * -Force
}
finally {
    Wait-BeforeExit
}

if ($exitCode -ne 0) {
    exit $exitCode
}
