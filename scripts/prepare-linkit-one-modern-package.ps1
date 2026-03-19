param(
    [string]$OutputRoot = "dist\linkit-one-modern-package"
)

$ErrorActionPreference = "Stop"

$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path

if ([System.IO.Path]::IsPathRooted($OutputRoot)) {
    $outputRootResolved = [System.IO.Path]::GetFullPath($OutputRoot)
} else {
    $outputRootResolved = [System.IO.Path]::GetFullPath((Join-Path $repoRoot $OutputRoot))
}
$platformRoot = Join-Path $outputRootResolved "hardware\mediatek\mtk"

$sourcePlatform = Join-Path $repoRoot "hardware\arduino\mtk"
$sourceArmToolchain = Join-Path $repoRoot "hardware\tools\g++_arm_none_eabi"
$sourceMtkTools = Join-Path $repoRoot "hardware\tools\mtk"
$sourceDrivers = Join-Path $repoRoot "drivers\mtk"

if (-not (Test-Path $sourcePlatform)) {
    throw "Missing source platform folder: $sourcePlatform"
}

if (Test-Path $outputRootResolved) {
    Remove-Item -Recurse -Force $outputRootResolved
}

New-Item -ItemType Directory -Force -Path $platformRoot | Out-Null
New-Item -ItemType Directory -Force -Path (Join-Path $platformRoot "tools") | Out-Null

Copy-Item -Recurse -Force $sourcePlatform\* $platformRoot
Copy-Item -Recurse -Force $sourceArmToolchain (Join-Path $platformRoot "tools\g++_arm_none_eabi")
Copy-Item -Recurse -Force $sourceMtkTools (Join-Path $platformRoot "tools\mtk")

if (Test-Path $sourceDrivers) {
    New-Item -ItemType Directory -Force -Path (Join-Path $outputRootResolved "drivers") | Out-Null
    Copy-Item -Recurse -Force $sourceDrivers (Join-Path $outputRootResolved "drivers")
}

$platformTxtPath = Join-Path $platformRoot "platform.txt"
$platformTxt = Get-Content $platformTxtPath -Raw

$platformTxt = $platformTxt.Replace(
    "name=Arduino ARM (32-bits) Boards",
    "name=MediaTek LinkIt ONE"
)
$platformTxt = $platformTxt.Replace(
    "version=1.5.6",
    "version=1.5.6-linkit-modern"
)
$platformTxt = $platformTxt.Replace(
    "compiler.path={runtime.ide.path}/hardware/tools/g++_arm_none_eabi/bin/",
    "compiler.path={runtime.platform.path}/tools/g++_arm_none_eabi/bin/"
)
$platformTxt = $platformTxt.Replace(
    'recipe.objcopy.hex.pattern="{runtime.ide.path}/hardware/tools/mtk/PackTag.exe" "{build.path}/{build.project_name}.elf" "{build.path}/{build.project_name}.vxp"',
    'recipe.objcopy.hex.pattern="{runtime.platform.path}/tools/mtk/PackTag.exe" "{build.path}/{build.project_name}.elf" "{build.path}/{build.project_name}.vxp"'
)
$platformTxt = $platformTxt.Replace(
    "tools.bossac.path={runtime.ide.path}/hardware/tools/mtk",
    "tools.bossac.path={runtime.platform.path}/tools/mtk"
)

Set-Content -Path $platformTxtPath -Value $platformTxt -Encoding ASCII

$readmePath = Join-Path $outputRootResolved "README_INSTALL.md"
$readme = @'
# LinkIt ONE manual package for modern Arduino IDE

This package was generated from the legacy LinkIt ONE IDE and repackaged for manual installation in newer Arduino IDE versions.

## What changed

- The platform keeps the original MediaTek core, libraries, firmware, and upload tools.
- Legacy references to `runtime.ide.path/hardware/tools/...` were rewritten to use `runtime.platform.path/tools/...`.
- The package is installed under `hardware/mediatek/mtk` so it does not collide with Arduino's built-in platforms.

## Install

1. Close Arduino IDE.
2. Copy the `hardware` folder from this package into your sketchbook folder.
3. On Windows, the default sketchbook folder is usually `C:\Users\<your-user>\Documents\Arduino`.
4. After copying, the platform path should look like:
   `C:\Users\<your-user>\Documents\Arduino\hardware\mediatek\mtk`
5. Reopen Arduino IDE.
6. Select board `LinkIt ONE`.

## Notes

- This is still a legacy Windows-only toolchain and uploader.
- Compatibility with Arduino IDE 2.x is best-effort. The package layout is modernized, but the core itself is still the old MediaTek implementation.
- If the board is not detected, install the bundled drivers from the `drivers\mtk` folder in this package.
'@

Set-Content -Path $readmePath -Value $readme -Encoding ASCII

Write-Output "Created package at: $outputRootResolved"
Write-Output "Install path target: <Sketchbook>\\hardware\\mediatek\\mtk"
