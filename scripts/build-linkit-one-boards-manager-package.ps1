param(
    [string]$BaseUrl = "https://example.com/linkit-one",
    [string]$Version = "1.5.6-linkit-modern.1",
    [string]$OutputRoot = "dist\boards-manager",
    [string]$PackagerName = "linkitone-community",
    [string]$Maintainer = "LinkIt ONE Community",
    [string]$WebsiteUrl = "",
    [string]$Email = ""
)

$ErrorActionPreference = "Stop"

$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$outputRootResolved = [System.IO.Path]::GetFullPath((Join-Path $repoRoot $OutputRoot))

$manualPackageRoot = Join-Path $repoRoot "dist\linkit-one-modern-package"
if (-not (Test-Path $manualPackageRoot)) {
    throw "Missing manual package at $manualPackageRoot. Run prepare-linkit-one-modern-package.ps1 first."
}

if ([string]::IsNullOrWhiteSpace($WebsiteUrl)) {
    $WebsiteUrl = $BaseUrl
}

if (Test-Path $outputRootResolved) {
    Remove-Item -Recurse -Force $outputRootResolved
}

New-Item -ItemType Directory -Force -Path $outputRootResolved | Out-Null
$stagingRoot = Join-Path $outputRootResolved "staging"
New-Item -ItemType Directory -Force -Path $stagingRoot | Out-Null

function New-ZipFromFolder {
    param(
        [string]$SourceFolder,
        [string]$ZipPath
    )

    if (Test-Path $ZipPath) {
        Remove-Item -Force $ZipPath
    }
    Compress-Archive -Path (Join-Path $SourceFolder "*") -DestinationPath $ZipPath
}

function Get-ArchiveMetadata {
    param(
        [string]$FilePath
    )

    $hash = Get-FileHash -Algorithm SHA256 -Path $FilePath
    $item = Get-Item $FilePath
    return @{
        fileName = $item.Name
        size = [string]$item.Length
        checksum = "SHA-256:" + $hash.Hash.ToUpperInvariant()
    }
}

# Platform archive layout:
# mtk/<boards.txt, platform.txt, cores, variants, libraries, system, programmers.txt>
$platformStaging = Join-Path $stagingRoot "platform\mtk"
New-Item -ItemType Directory -Force -Path $platformStaging | Out-Null

$platformSource = Join-Path $manualPackageRoot "hardware\mediatek\mtk"
foreach ($name in @("boards.txt", "platform.txt", "programmers.txt", "cores", "variants", "libraries", "system")) {
    $source = Join-Path $platformSource $name
    if (Test-Path $source) {
        Copy-Item -Recurse -Force $source $platformStaging
    }
}

$platformArchive = Join-Path $outputRootResolved "linkit-one-platform-$Version.zip"
New-ZipFromFolder -SourceFolder (Join-Path $stagingRoot "platform") -ZipPath $platformArchive

# ARM toolchain archive layout:
# g++_arm_none_eabi/<bin, lib, libexec, ...>
$armToolStaging = Join-Path $stagingRoot "tool-arm\g++_arm_none_eabi"
New-Item -ItemType Directory -Force -Path $armToolStaging | Out-Null
Copy-Item -Recurse -Force (Join-Path $platformSource "tools\g++_arm_none_eabi\*") $armToolStaging

$armToolArchive = Join-Path $outputRootResolved "linkit-one-toolchain-arm-none-eabi-$Version.zip"
New-ZipFromFolder -SourceFolder (Join-Path $stagingRoot "tool-arm") -ZipPath $armToolArchive

# MTK uploader archive layout:
# mtk/<PushTool.exe, PackTag.exe, firmware, ...>
$mtkToolStaging = Join-Path $stagingRoot "tool-mtk\mtk"
New-Item -ItemType Directory -Force -Path $mtkToolStaging | Out-Null
Copy-Item -Recurse -Force (Join-Path $platformSource "tools\mtk\*") $mtkToolStaging

$mtkToolArchive = Join-Path $outputRootResolved "linkit-one-tool-mtk-$Version.zip"
New-ZipFromFolder -SourceFolder (Join-Path $stagingRoot "tool-mtk") -ZipPath $mtkToolArchive

$platformMeta = Get-ArchiveMetadata -FilePath $platformArchive
$armToolMeta = Get-ArchiveMetadata -FilePath $armToolArchive
$mtkToolMeta = Get-ArchiveMetadata -FilePath $mtkToolArchive

$indexFileName = "package_${PackagerName}_index.json"
$indexPath = Join-Path $outputRootResolved $indexFileName

$json = @"
{
  "packages": [
    {
      "name": "mediatek",
      "maintainer": "$Maintainer",
      "websiteURL": "$WebsiteUrl",
      "email": "$Email",
      "help": {
        "online": "$WebsiteUrl"
      },
      "platforms": [
        {
          "name": "MediaTek LinkIt ONE",
          "architecture": "mtk",
          "version": "$Version",
          "category": "Contributed",
          "help": {
            "online": "$WebsiteUrl"
          },
          "url": "$BaseUrl/$($platformMeta.fileName)",
          "archiveFileName": "$($platformMeta.fileName)",
          "checksum": "$($platformMeta.checksum)",
          "size": "$($platformMeta.size)",
          "boards": [
            {
              "name": "LinkIt ONE"
            }
          ],
          "toolsDependencies": [
            {
              "packager": "mediatek",
              "name": "g++_arm_none_eabi",
              "version": "$Version"
            },
            {
              "packager": "mediatek",
              "name": "mtk",
              "version": "$Version"
            }
          ]
        }
      ],
      "tools": [
        {
          "name": "g++_arm_none_eabi",
          "version": "$Version",
          "systems": [
            {
              "host": "x86_64-mingw32",
              "url": "$BaseUrl/$($armToolMeta.fileName)",
              "archiveFileName": "$($armToolMeta.fileName)",
              "checksum": "$($armToolMeta.checksum)",
              "size": "$($armToolMeta.size)"
            },
            {
              "host": "i686-mingw32",
              "url": "$BaseUrl/$($armToolMeta.fileName)",
              "archiveFileName": "$($armToolMeta.fileName)",
              "checksum": "$($armToolMeta.checksum)",
              "size": "$($armToolMeta.size)"
            }
          ]
        },
        {
          "name": "mtk",
          "version": "$Version",
          "systems": [
            {
              "host": "x86_64-mingw32",
              "url": "$BaseUrl/$($mtkToolMeta.fileName)",
              "archiveFileName": "$($mtkToolMeta.fileName)",
              "checksum": "$($mtkToolMeta.checksum)",
              "size": "$($mtkToolMeta.size)"
            },
            {
              "host": "i686-mingw32",
              "url": "$BaseUrl/$($mtkToolMeta.fileName)",
              "archiveFileName": "$($mtkToolMeta.fileName)",
              "checksum": "$($mtkToolMeta.checksum)",
              "size": "$($mtkToolMeta.size)"
            }
          ]
        }
      ]
    }
  ]
}
"@

Set-Content -Path $indexPath -Value $json -Encoding ASCII

$readmePath = Join-Path $outputRootResolved "README_BOARDS_MANAGER.md"
$readme = @"
# LinkIt ONE Boards Manager package

This folder contains a complete Arduino Boards Manager distribution for the adapted LinkIt ONE package.

## Files

- $indexFileName
- $($platformMeta.fileName)
- $($armToolMeta.fileName)
- $($mtkToolMeta.fileName)

## How to publish

1. Upload all files in this folder to a public HTTP or HTTPS location.
2. Replace the default `BaseUrl` if needed by rerunning:

   `powershell -ExecutionPolicy Bypass -File .\scripts\build-linkit-one-boards-manager-package.ps1 -BaseUrl "https://your-host/path"`

3. Share the JSON URL:

   $($BaseUrl)/$indexFileName

4. In Arduino IDE, users add that URL under:

   `File > Preferences > Additional boards manager URLs`

## Notes

- This package is Windows-only because the bundled MediaTek upload tools are Windows executables.
- Users should remove old manual installs of `hardware\arduino\mtk` or `hardware\mediatek\mtk` before switching to Boards Manager installation.
"@

Set-Content -Path $readmePath -Value $readme -Encoding ASCII

$repoCopyPath = Join-Path $repoRoot $indexFileName
Copy-Item -Force $indexPath $repoCopyPath

Write-Output "Boards Manager package created at: $outputRootResolved"
Write-Output "Index file: $indexPath"
Write-Output "Repo-root index copy: $repoCopyPath"
