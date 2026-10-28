[CmdletBinding()]
param(
    [ValidateSet('Debug', 'Release', 'Both')]
    [string]$Configuration = 'Both',

    [switch]$Clean
)

$ErrorActionPreference = 'Stop'

$editorRoot = $PSScriptRoot
$projectRoot = [System.IO.Path]::GetFullPath((Join-Path $editorRoot '..\..'))
$buildRoot = [System.IO.Path]::GetFullPath((Join-Path $editorRoot 'build\windows-vs2022-x64'))
$expectedBuildParent = [System.IO.Path]::GetFullPath((Join-Path $editorRoot 'build'))

if ($Clean -and (Test-Path -LiteralPath $buildRoot)) {
    if (-not $buildRoot.StartsWith($expectedBuildParent + [System.IO.Path]::DirectorySeparatorChar,
            [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "Refusing to clean a build path outside the Map Editor directory: $buildRoot"
    }
    Remove-Item -LiteralPath $buildRoot -Recurse -Force
}

Write-Host "Configuring Phoenix Map Editor"
Write-Host "  Source: $projectRoot"
Write-Host "  Build:  $buildRoot"

& cmake -S $projectRoot -B $buildRoot -G 'Visual Studio 17 2022' -A x64
if ($LASTEXITCODE -ne 0) {
    throw "Map Editor CMake configuration failed with exit code $LASTEXITCODE."
}

$configurations = if ($Configuration -eq 'Both') { @('Debug', 'Release') } else { @($Configuration) }
foreach ($activeConfiguration in $configurations) {
    Write-Host "Building PhoenixMapEditor ($activeConfiguration)"
    & cmake --build $buildRoot --config $activeConfiguration --target PhoenixMapEditor --parallel
    if ($LASTEXITCODE -ne 0) {
        throw "Map Editor $activeConfiguration build failed with exit code $LASTEXITCODE."
    }

    $binaryName = if ($activeConfiguration -eq 'Debug') {
        'PhoenixMapEditor_Debug.exe'
    } else {
        'PhoenixMapEditor_Release.exe'
    }
    $binaryPath = Join-Path $editorRoot "bin\$binaryName"
    if (-not (Test-Path -LiteralPath $binaryPath -PathType Leaf)) {
        throw "Map Editor build completed without producing $binaryPath."
    }
    Write-Host "Built: $binaryPath"
}

Write-Host 'Phoenix Map Editor build completed. The application was not launched.'

