param(
    [switch] $Clean,
    [switch] $NoPause
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

$root = $PSScriptRoot
$buildRoot = Join-Path $root "build"
$buildDir = Join-Path $root "build"
$mapEditorExe = Join-Path $root "Tool\MapEditor\bin\PhoenixMapEditor_Release.exe"

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

function Invoke-MapEditorBuild {
    $runningEditor = Get-Process -Name "PhoenixMapEditor*" -ErrorAction SilentlyContinue
    if ($runningEditor) {
        $ids = ($runningEditor | ForEach-Object { "$($_.ProcessName):$($_.Id)" }) -join ", "
        throw "PhoenixMapEditor is running and may lock the output exe. Close it first. Running: $ids"
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
            Invoke-Checked cmake -B $buildDir -S $root
        }

        # Only build PhoenixMapEditor target (not PhoenixClient)
        Invoke-Checked cmake --build $buildDir --config Release --target PhoenixMapEditor

        if (-not (Test-Path $mapEditorExe)) {
            throw "MapEditor output was not produced: $mapEditorExe"
        }

        $file = Get-Item $mapEditorExe
        Write-Host "MapEditor build complete:"
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
    Invoke-MapEditorBuild
}
catch {
    $exitCode = 1
    Write-Host ""
    Write-Host "MapEditor build failed:" -ForegroundColor Red
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
