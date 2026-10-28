param(
    [string]$ProjectRoot = "E:\DK Phoenix Reborn",
    [switch]$Force,
    [switch]$VerifyOnly
)

$ErrorActionPreference = "Stop"

$root = [System.IO.Path]::GetFullPath($ProjectRoot)
$assetsRoot = Join-Path $root "Assets\Fonts"
$manifestRoot = Join-Path $assetsRoot "Manifest"
$fontStackRoot = Join-Path $assetsRoot "FontStack"
$licenseRoot = Join-Path $root "ThirdParty\Licenses\Fonts\Noto"
$validationRoot = Join-Path $root "Docs\ValidationReports"

$directories = @(
    "Assets\Fonts\Noto\NotoSans",
    "Assets\Fonts\Noto\NotoSansCJK\KR",
    "Assets\Fonts\Noto\NotoSansCJK\JP",
    "Assets\Fonts\Noto\NotoSansCJK\SC",
    "Assets\Fonts\Noto\NotoSansCJK\TC",
    "Assets\Fonts\Noto\NotoSansCJK\HK",
    "Assets\Fonts\Noto\NotoSansSymbols2",
    "Assets\Fonts\Noto\NotoSansMono",
    "Assets\Fonts\FontStack",
    "Assets\Fonts\Manifest",
    "ThirdParty\Licenses\Fonts\Noto",
    "Docs\ValidationReports"
)

foreach ($directory in $directories) {
    New-Item -ItemType Directory -Force -Path (Join-Path $root $directory) | Out-Null
}

$fonts = @(
    @{ Id = "noto_sans"; DisplayName = "Noto Sans"; Role = "primary_ui"; Destination = "Assets\Fonts\Noto\NotoSans\NotoSans[wdth,wght].ttf"; Url = "https://raw.githubusercontent.com/google/fonts/main/ofl/notosans/NotoSans%5Bwdth,wght%5D.ttf" },
    @{ Id = "noto_sans_italic"; DisplayName = "Noto Sans Italic"; Role = "primary_ui_italic"; Destination = "Assets\Fonts\Noto\NotoSans\NotoSans-Italic[wdth,wght].ttf"; Url = "https://raw.githubusercontent.com/google/fonts/main/ofl/notosans/NotoSans-Italic%5Bwdth,wght%5D.ttf" },
    @{ Id = "noto_sans_kr"; DisplayName = "Noto Sans KR"; Role = "fallback_korean"; Destination = "Assets\Fonts\Noto\NotoSansCJK\KR\NotoSansKR[wght].ttf"; Url = "https://raw.githubusercontent.com/google/fonts/main/ofl/notosanskr/NotoSansKR%5Bwght%5D.ttf" },
    @{ Id = "noto_sans_jp"; DisplayName = "Noto Sans JP"; Role = "fallback_japanese"; Destination = "Assets\Fonts\Noto\NotoSansCJK\JP\NotoSansJP[wght].ttf"; Url = "https://raw.githubusercontent.com/google/fonts/main/ofl/notosansjp/NotoSansJP%5Bwght%5D.ttf" },
    @{ Id = "noto_sans_sc"; DisplayName = "Noto Sans SC"; Role = "fallback_simplified_chinese"; Destination = "Assets\Fonts\Noto\NotoSansCJK\SC\NotoSansSC[wght].ttf"; Url = "https://raw.githubusercontent.com/google/fonts/main/ofl/notosanssc/NotoSansSC%5Bwght%5D.ttf" },
    @{ Id = "noto_sans_tc"; DisplayName = "Noto Sans TC"; Role = "fallback_traditional_chinese"; Destination = "Assets\Fonts\Noto\NotoSansCJK\TC\NotoSansTC[wght].ttf"; Url = "https://raw.githubusercontent.com/google/fonts/main/ofl/notosanstc/NotoSansTC%5Bwght%5D.ttf" },
    @{ Id = "noto_sans_hk"; DisplayName = "Noto Sans HK"; Role = "fallback_hong_kong"; Destination = "Assets\Fonts\Noto\NotoSansCJK\HK\NotoSansHK[wght].ttf"; Url = "https://raw.githubusercontent.com/google/fonts/main/ofl/notosanshk/NotoSansHK%5Bwght%5D.ttf" },
    @{ Id = "noto_sans_symbols2"; DisplayName = "Noto Sans Symbols 2"; Role = "fallback_symbols"; Destination = "Assets\Fonts\Noto\NotoSansSymbols2\NotoSansSymbols2-Regular.ttf"; Url = "https://raw.githubusercontent.com/google/fonts/main/ofl/notosanssymbols2/NotoSansSymbols2-Regular.ttf" },
    @{ Id = "noto_sans_mono"; DisplayName = "Noto Sans Mono"; Role = "debug_monospace"; Destination = "Assets\Fonts\Noto\NotoSansMono\NotoSansMono[wdth,wght].ttf"; Url = "https://raw.githubusercontent.com/google/fonts/main/ofl/notosansmono/NotoSansMono%5Bwdth,wght%5D.ttf" }
)

$licenses = @(
    @{ Id = "notosans"; Destination = "ThirdParty\Licenses\Fonts\Noto\OFL_notosans.txt"; Url = "https://raw.githubusercontent.com/google/fonts/main/ofl/notosans/OFL.txt" },
    @{ Id = "notosanskr"; Destination = "ThirdParty\Licenses\Fonts\Noto\OFL_notosanskr.txt"; Url = "https://raw.githubusercontent.com/google/fonts/main/ofl/notosanskr/OFL.txt" },
    @{ Id = "notosansjp"; Destination = "ThirdParty\Licenses\Fonts\Noto\OFL_notosansjp.txt"; Url = "https://raw.githubusercontent.com/google/fonts/main/ofl/notosansjp/OFL.txt" },
    @{ Id = "notosanssc"; Destination = "ThirdParty\Licenses\Fonts\Noto\OFL_notosanssc.txt"; Url = "https://raw.githubusercontent.com/google/fonts/main/ofl/notosanssc/OFL.txt" },
    @{ Id = "notosanstc"; Destination = "ThirdParty\Licenses\Fonts\Noto\OFL_notosanstc.txt"; Url = "https://raw.githubusercontent.com/google/fonts/main/ofl/notosanstc/OFL.txt" },
    @{ Id = "notosanshk"; Destination = "ThirdParty\Licenses\Fonts\Noto\OFL_notosanshk.txt"; Url = "https://raw.githubusercontent.com/google/fonts/main/ofl/notosanshk/OFL.txt" },
    @{ Id = "notosanssymbols2"; Destination = "ThirdParty\Licenses\Fonts\Noto\OFL_notosanssymbols2.txt"; Url = "https://raw.githubusercontent.com/google/fonts/main/ofl/notosanssymbols2/OFL.txt" },
    @{ Id = "notosansmono"; Destination = "ThirdParty\Licenses\Fonts\Noto\OFL_notosansmono.txt"; Url = "https://raw.githubusercontent.com/google/fonts/main/ofl/notosansmono/OFL.txt" }
)

function Download-FileIfNeeded {
    param(
        [string]$Url,
        [string]$Destination
    )

    if ($VerifyOnly) {
        return
    }

    if ((Test-Path -LiteralPath $Destination) -and -not $Force) {
        return
    }

    $tempPath = Join-Path ([System.IO.Path]::GetTempPath()) ([System.IO.Path]::GetRandomFileName())
    try {
        Invoke-WebRequest -Uri $Url -OutFile $tempPath -UseBasicParsing
        [System.IO.File]::Copy($tempPath, $Destination, $true)
    }
    finally {
        if (Test-Path -LiteralPath $tempPath) {
            Remove-Item -LiteralPath $tempPath -Force
        }
    }
}

function Get-FileRecord {
    param(
        [string]$RelativePath,
        [string]$SourceUrl
    )

    $path = Join-Path $root $RelativePath
    if (-not (Test-Path -LiteralPath $path)) {
        return @{
            path = $RelativePath.Replace("\", "/")
            sourceUrl = $SourceUrl
            sha256 = $null
            sizeBytes = 0
            status = "missing"
        }
    }

    $item = Get-Item -LiteralPath $path
    $hash = Get-FileHash -Algorithm SHA256 -LiteralPath $path
    return @{
        path = $RelativePath.Replace("\", "/")
        sourceUrl = $SourceUrl
        sha256 = $hash.Hash.ToLowerInvariant()
        sizeBytes = [Int64]$item.Length
        status = $(if ($item.Length -gt 0) { "ok" } else { "empty" })
    }
}

$downloadErrors = @()

foreach ($font in $fonts) {
    $destination = Join-Path $root $font.Destination
    try {
        Download-FileIfNeeded -Url $font.Url -Destination $destination
    }
    catch {
        $downloadErrors += "Font $($font.Id): $($_.Exception.Message)"
    }
}

foreach ($license in $licenses) {
    $destination = Join-Path $root $license.Destination
    try {
        Download-FileIfNeeded -Url $license.Url -Destination $destination
    }
    catch {
        $downloadErrors += "License $($license.Id): $($_.Exception.Message)"
    }
}

$canonicalOfl = Join-Path $licenseRoot "OFL.txt"
$firstOfl = Join-Path $root "ThirdParty\Licenses\Fonts\Noto\OFL_notosans.txt"
if ((Test-Path -LiteralPath $firstOfl) -and ((-not (Test-Path -LiteralPath $canonicalOfl)) -or $Force)) {
    Copy-Item -LiteralPath $firstOfl -Destination $canonicalOfl -Force
}

$readmePath = Join-Path $licenseRoot "README_Fonts.md"
$readme = @"
# Noto Font Licenses

Fonts in this directory are downloaded from the official Google Fonts repository.

License: SIL Open Font License 1.1.

Segoe UI Emoji is used only as a Windows system fallback and is not copied into this project.
"@
Set-Content -Path $readmePath -Value $readme -Encoding UTF8

$families = @()
foreach ($font in $fonts) {
    $families += @{
        id = $font.Id
        displayName = $font.DisplayName
        role = $font.Role
        localFiles = @(Get-FileRecord -RelativePath $font.Destination -SourceUrl $font.Url)
    }
}

$families += @{
    id = "segoe_ui_emoji"
    displayName = "Segoe UI Emoji"
    role = "system_emoji"
    localFiles = @()
    systemFont = $true
}

$manifest = @{
    schemaVersion = 1
    generatedAtUtc = (Get-Date).ToUniversalTime().ToString("o")
    license = "SIL Open Font License 1.1"
    families = $families
    licenses = @($licenses | ForEach-Object {
        Get-FileRecord -RelativePath $_.Destination -SourceUrl $_.Url
    })
    downloadErrors = $downloadErrors
}

$manifestPath = Join-Path $manifestRoot "fonts_manifest.json"
$manifest | ConvertTo-Json -Depth 8 | Set-Content -Path $manifestPath -Encoding UTF8

$fontStack = @{
    schemaVersion = 1
    defaultFontStack = @("Noto Sans", "Noto Sans KR", "Noto Sans JP", "Noto Sans SC", "Noto Sans TC", "Noto Sans HK", "Noto Sans Symbols 2", "Segoe UI Emoji")
    ui = @{
        regular = "Noto Sans"
        medium = "Noto Sans"
        semibold = "Noto Sans"
        bold = "Noto Sans"
    }
    chat = @{
        regular = "Noto Sans"
        fallback = @("Noto Sans KR", "Noto Sans JP", "Noto Sans SC", "Noto Sans TC", "Noto Sans Symbols 2", "Segoe UI Emoji")
    }
    debug = @{
        regular = "Noto Sans Mono"
    }
    emoji = @{
        systemFallback = "Segoe UI Emoji"
    }
}

$fontStackPath = Join-Path $fontStackRoot "dkphoenix_font_stack.json"
$fontStack | ConvertTo-Json -Depth 8 | Set-Content -Path $fontStackPath -Encoding UTF8

$testStrings = @"
Turkish:
Merhaba Dunya - CGIOSU cgiosu iI
Merhaba Dünya - ÇĞİÖŞÜ çğıöşü ıİ

English:
The quick brown fox jumps over the lazy dog.

Korean:
안녕하세요 다크에덴 테스트

Japanese:
こんにちは ダークエデン テスト

Simplified Chinese:
你好，暗黑伊甸园测试

Traditional Chinese:
你好，闇黑伊甸園測試

Symbols:
★ ☆ ▲ ▼ ◆ ◇ → ← ↑ ↓ ✓ ✗ ⚔ ⚡

Numbers:
0123456789

Mixed UI:
HP 100/100 - MP 75/100 - STR +10 - Skill Lv. 5

Chat:
[Guild] PlayerName: Merhaba 안녕하세요 こんにちは 你好

Tooltip:
Rare Item ★ 공격력 +15 / Savunma +20
"@

$testStringPath = Join-Path $fontStackRoot "dkphoenix_font_test_strings.txt"
Set-Content -Path $testStringPath -Value $testStrings -Encoding UTF8

$fontRecords = @()
foreach ($family in $manifest["families"]) {
    if ($family.ContainsKey("localFiles")) {
        foreach ($file in $family["localFiles"]) {
            $fontRecords += $file
        }
    }
}

$downloadedFonts = @($fontRecords | Where-Object { $_["status"] -eq "ok" })
$missingFonts = @($fontRecords | Where-Object { $_["status"] -ne "ok" })
$licenseRecords = @($manifest["licenses"])
$validLicenses = @($licenseRecords | Where-Object { $_["status"] -eq "ok" })
$totalSize = [Int64]0
foreach ($fontRecord in $downloadedFonts) {
    $totalSize += [Int64]$fontRecord["sizeBytes"]
}

$hashLines = @()
foreach ($family in $manifest["families"]) {
    if (-not $family.ContainsKey("localFiles")) {
        continue
    }

    foreach ($file in $family["localFiles"]) {
        $hashLines += "- {0}: `{1}` sha256={2} size={3} status={4}" -f $family["displayName"], $file["path"], $file["sha256"], $file["sizeBytes"], $file["status"]
    }
}

$warningLines = @()
if ($missingFonts.Count -gt 0) {
    $warningLines += "- Missing or empty font files: $($missingFonts.Count)"
}
if ($downloadErrors.Count -gt 0) {
    foreach ($errorLine in $downloadErrors) {
        $warningLines += "- $errorLine"
    }
}
$warningLines += "- CJK fallback fonts are large; installer/package size must account for them."
$warningLines += "- Glyph coverage validation is future work; this phase validates file presence, size, hashes, manifest, stack config, and licenses."

$validation = @"
# Font Validation

- Downloaded font count: $($downloadedFonts.Count)
- Missing font count: $($missingFonts.Count)
- Total font size: $totalSize bytes
- License status: $($validLicenses.Count) / $($licenses.Count) license files present
- Font stack config: $($fontStackPath.Replace($root, "").TrimStart("\"))
- Manifest: $($manifestPath.Replace($root, "").TrimStart("\"))
- Test strings: $($testStringPath.Replace($root, "").TrimStart("\"))
- CJK fallback availability: KR, JP, SC, TC, HK included when present in manifest as status ok
- Symbols availability: Noto Sans Symbols 2 included when present as status ok
- System emoji fallback status: Segoe UI Emoji configured as system fallback; no local emoji font copied

## Per-font SHA256

$($hashLines -join "`r`n")

## Warnings

$($warningLines -join "`r`n")

## Next steps

- Add DirectWrite-backed runtime font loading.
- Add glyph coverage validation.
- Add text layout and glyph atlas caches for DX11/DX12 rendering.
"@

$validationPath = Join-Path $validationRoot "FontValidation.md"
Set-Content -Path $validationPath -Value $validation -Encoding UTF8

if ($missingFonts.Count -gt 0 -or $downloadErrors.Count -gt 0) {
    Write-Host "Font setup completed with warnings. Missing fonts: $($missingFonts.Count)."
    exit 2
}

Write-Host "Font setup completed. Fonts: $($downloadedFonts.Count). Total bytes: $totalSize."
