# LinkIt ONE Handover

## Project state

This repository is being used to rebuild a usable modern Arduino IDE package for the MediaTek LinkIt ONE.

The original slimmed-down package in this repo was missing major parts of the legacy SDK, especially the LinkIt-specific libraries and tools needed for Bluetooth, GPS, GSM, Wi-Fi, and related features.

This handover is separate from the release-oriented notes in `RELEASE_NOTES_v1.5.6-linkit-modern.1.md`.

The release notes are still relevant background because they describe the intended packaging changes around the modern Arduino IDE layout, bundled uploader/toolchain assets, builder path fixes, and variant updates. They do not list the full restored SDK payload in detail, so this handover complements them with the current repo state.

## User goal

The immediate user goal was:

- Make LinkIt ONE act as a Bluetooth host
- View communication through Serial Monitor
- Connect specifically to a PASCO AirLink device

## Important technical finding

PASCO AirLink appears to be a BLE device, not a classic Bluetooth SPP device.

The restored LinkIt ONE Arduino SDK in this workspace exposes:

- `LBT` for classic Bluetooth SPP
- `LGPRS`, `LGPS`, `LGSM`, `LWiFi`, and other LinkIt libraries

But it does not expose Arduino-level BLE GATT wrapper headers such as:

- `LGATT.h`
- `LGATTClient.h`
- `LGATTServer.h`

This means:

- LinkIt ONE can likely act as a classic Bluetooth SPP host with `LBT`
- LinkIt ONE cannot currently be used here as an Arduino-level BLE central for PASCO AirLink using the recovered SDK

## What was copied into this repo

Source used:

- `C:\Users\jared salazar\OneDrive\Documents\GitHub\LinkIt-ONE-IDE`

Copied into this repo:

- `hardware/arduino/mtk/*`
- `hardware/tools/g++_arm_none_eabi`
- `hardware/tools/mtk`
- `drivers/mtk`

Destination repo:

- `C:\Users\jared salazar\OneDrive\Documents\GitHub\Linkitone_Arduino`

## Files of interest

Modern package repo:

- `C:\Users\jared salazar\OneDrive\Documents\GitHub\Linkitone_Arduino\README.md`
- `C:\Users\jared salazar\OneDrive\Documents\GitHub\Linkitone_Arduino\MODERN_IDE_ADAPTATION.md`
- `C:\Users\jared salazar\OneDrive\Documents\GitHub\Linkitone_Arduino\BOARDS_MANAGER_PACKAGING.md`
- `C:\Users\jared salazar\OneDrive\Documents\GitHub\Linkitone_Arduino\RELEASE_NOTES_v1.5.6-linkit-modern.1.md`
- `C:\Users\jared salazar\OneDrive\Documents\GitHub\Linkitone_Arduino\examples\PascoAirLinkProbe\PascoAirLinkProbe.ino`

Release-notes context:

- `RELEASE_NOTES_v1.5.6-linkit-modern.1.md` documents the package-level modernization work:
- adapted legacy LinkIt ONE platform for modern Arduino IDE package layout
- bundled MediaTek uploader and ARM toolchain as Boards Manager tools
- builder path compatibility fixes
- `LED_BUILTIN` addition for the LinkIt ONE variant

Recovered LinkIt libraries:

- `C:\Users\jared salazar\OneDrive\Documents\GitHub\Linkitone_Arduino\hardware\arduino\mtk\libraries\LBT\LBT.h`
- `C:\Users\jared salazar\OneDrive\Documents\GitHub\Linkitone_Arduino\hardware\arduino\mtk\libraries\LBT\LBTClient.h`
- `C:\Users\jared salazar\OneDrive\Documents\GitHub\Linkitone_Arduino\hardware\arduino\mtk\libraries\LBT\LBTServer.h`

Reference example:

- `C:\Users\jared salazar\OneDrive\Documents\GitHub\Linkitone_Arduino\hardware\arduino\mtk\libraries\LBT\examples\BTSPP\BTClient\BTClient.ino`

## Current sketch status

`examples/PascoAirLinkProbe/PascoAirLinkProbe.ino` was originally written as a BLE probe sketch. It now contains an explicit compile-time error explaining that the current SDK is missing `LGATT` headers.

That sketch is not expected to compile unless BLE Arduino wrapper libraries are found and restored.

## What was verified

- The old legacy IDE folder does contain the full `hardware/arduino/mtk` platform and tool folders.
- The old legacy IDE folder does contain the classic Bluetooth `LBT` library.
- The old legacy IDE folder does not appear to contain `LGATT.h` / `LGATTClient.h` files.
- The LinkIt system headers include low-level Bluetooth/GATT-related headers under `system/libmtk/include`, but no confirmed Arduino wrapper layer for BLE was found during this pass.

## Likely next paths

1. If the target can switch to classic Bluetooth SPP:
   build an `LBTClient`-based host sketch and mirror traffic to Serial.

2. If PASCO AirLink is strictly BLE:
   either find a missing BLE Arduino wrapper package for LinkIt ONE, or move the task to a board with clear BLE central support such as ESP32.

3. Rebuild the package archives after validating the restored platform layout:
   use the PowerShell packaging scripts already in this repo.

## Suggested next action for the next Codex

Start by confirming the current Arduino install path and whether the restored local package is the one the IDE is actually compiling against.

Then choose one of:

- make a working classic Bluetooth `LBT` host test sketch
- continue investigating whether a missing BLE wrapper exists outside the recovered legacy IDE
- recommend migration to ESP32 for PASCO AirLink BLE work
