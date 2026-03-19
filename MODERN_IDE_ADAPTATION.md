# LinkIt ONE modern Arduino IDE adaptation

This repository contains a legacy LinkIt ONE Arduino IDE distribution based on Arduino 1.5.6. The original platform expects its compiler and uploader tools to live under the IDE installation directory:

- `hardware/arduino/mtk/platform.txt`
- `hardware/tools/g++_arm_none_eabi`
- `hardware/tools/mtk`

Modern Arduino IDE versions still support manual platform installation in the sketchbook `hardware` folder, but they do not provide the old `hardware/tools/...` layout expected by this package.

## Included adaptation

Use:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\prepare-linkit-one-modern-package.ps1
```

This generates:

```text
dist\linkit-one-modern-package\
  hardware\mediatek\mtk\...
  drivers\mtk\...
  README_INSTALL.md
```

## What the script patches

- Rewrites the compiler path to `{runtime.platform.path}/tools/g++_arm_none_eabi/bin/`
- Rewrites the packer path to `{runtime.platform.path}/tools/mtk/PackTag.exe`
- Rewrites the uploader tool path to `{runtime.platform.path}/tools/mtk`
- Renames the platform display name to `MediaTek LinkIt ONE`

## Limitations

- The package remains based on the legacy MediaTek core and uploader.
- The uploader and drivers in this repo are Windows-specific.
- This is a manual-install package, not a fully maintained Boards Manager package.
- I could not compile-test it in this environment because `arduino-cli` is not installed here.
