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

---

## Addendum: ESP32 + PASCO AirLink BLE track

This section was added later and should not replace the LinkIt ONE notes above. The active reverse-engineering work moved to ESP32 because LinkIt ONE Arduino BLE support remained unavailable.

## Current active goal

- Connect an ESP32 to PASCO AirLink over BLE
- Inspect the AirLink BLE protocol
- Get as close as possible to the raw sensor reading for a PASPORT Soil Moisture sensor connected through the AirLink

## Active sketch

- `C:\Users\jared salazar\OneDrive\Documents\GitHub\Linkitone_Arduino\examples\esp32PascoAirlink\esp32PascoAirlink.ino`

This sketch currently:

- scans for BLE devices
- matches the AirLink by name and MAC
- connects as BLE central
- enumerates services and characteristics
- subscribes to the PASCO custom characteristics
- logs packets to Serial
- decodes observed `0x82` and `0x85` packet families
- supports Serial commands such as `help`, `targets`, `probe`, `probe0`, `probe1`, `pasco`, `pasco0`, and `pasco1`

## AirLink identity and BLE map

Target device found in scans:

- Name: `AirLink 637-103>00`
- MAC: `80:6f:b0:74:88:6f`

Custom PASCO services discovered:

- `4a5c0000-0000-0000-0000-5c1e741f1c00`
- `4a5c0001-0000-0000-0000-5c1e741f1c00`

Important characteristics seen by the ESP32 sketch:

- `4a5c0000-0002-0000-0000-5c1e741f1c00`
- `4a5c0000-0003-0000-0000-5c1e741f1c00`
- `4a5c0001-0002-0000-0000-5c1e741f1c00`
- `4a5c0001-0003-0000-0000-5c1e741f1c00`
- `4a5c0001-0004-0000-0000-5c1e741f1c00`
- `4a5c0001-0005-0000-0000-5c1e741f1c00`

Internal labels used in the sketch:

- `pasco-write-0` = `4a5c0000-...-0003`
- `pasco-write-1` = `4a5c0001-...-0003`
- `pasco-notify-0` = `4a5c0000-...-0002`
- `pasco-notify-1` = `4a5c0001-...-0002`
- `pasco-read-notify` = `4a5c0001-...-0004`

Important correction discovered later:

- those internal labels in the sketch are likely wrong for PASCO protocol semantics
- the published `pasco-ble` package indicates the intended roles are:
- `...0002` = send command
- `...0003` = receive response / notifications
- `...0005` = send acknowledgement
- this means the current ESP32 sketch has probably been sending commands to the wrong characteristic (`...0003`)

## Observed packet families

### `0x82` packet

Observed on `pasco-write-1`:

- `82 01 01`

The sketch currently prints:

- `Decoded 0x82 frame: a=1 b=1`

Based on PASCO Python source, this is very likely AirLink sensor-ID related.

### `0x85` packet

Recurring six-byte frame observed mainly on `pasco-write-0`:

- example: `85 75 0F 5B 00 00`

The sketch currently decodes it as:

- `likelyRawADC = byte1 + (byte2 << 8)`
- `normalized = likelyRawADC / 4095.0`
- `stateByte = byte3`
- `flags = byte4, byte5`

Current best interpretation:

- the two-byte field in `0x85` is the best candidate for the raw ADC-like sensor reading
- it is likely raw or semi-raw PASPORT data, not the fully interpreted `%VWC` value shown by SPARKvue

## Soil Moisture sensor context from SPARKvue

User connected the AirLink successfully in SPARKvue on the PC.

Observed measurement channels in SPARKvue:

- `VWC Potting Soil`
- `VWC Mineral Soil`
- `VWC Rockwool`
- `Water Potential`
- `Voltage`

Local SPARKvue install files inspected:

- `C:\Program Files (x86)\PASCO scientific\Common Files\Sensors\Soil_Moisture.sds`
- `C:\Program Files (x86)\PASCO scientific\Common Files\Sensors\datasheets.xml`
- `C:\Program Files (x86)\PASCO scientific\Common Files\Sensors\DatasheetStrings_enu.txt`

These confirm the soil moisture sensor definitions and support the idea that PASCO applies sensor-specific metadata/calibration above the raw transport layer.

## PASCO Python clue

Repository checked:

- `https://github.com/PASCOscientific/pasco_python`

Important findings from `pasco_ble_device.py`:

- same PASCO UUID family: `4a5c000<service>-000<char>-0000-0000-5c1e741f1c00`
- `GEVT_SENSOR_ID = 0x82  # Get Sensor ID (for AirLink)`
- `GCMD_READ_ONE_SAMPLE = 0x05`
- `GCMD_XFER_BURST_RAM = 0x0E`
- `GCMD_CUSTOM_CMD = 0x37`

This strongly suggests the AirLink is using the same PASCO BLE framing family as the public Python library, even though the AirLink itself is not directly documented as a public developer API.

## PASCO npm / TypeScript clue

Package checked:

- `https://www.npmjs.com/package/pasco-ble`

The published package contents were downloaded and inspected locally from the npm tarball.

Important findings from the extracted files:

- `protocol-handler.js` confirms:
- `SENSOR_SERVICE_ID = 0`
- `SEND_CMD_CHAR_ID = 2`
- `RECV_CMD_CHAR_ID = 3`
- `SEND_ACK_CHAR_ID = 5`
- `GCMD_READ_ONE_SAMPLE = 0x05`
- `GCMD_XFER_BURST_RAM = 0x0E`
- `GCMD_CUSTOM_CMD = 0x37`
- `GEVT_SENSOR_ID = 0x82`

- `pasco-ble-device.js` confirms:
- notifications from service IDs greater than `0` are treated as sensor measurement responses
- service `0` is treated as the device-response path

- `sensor-manager.js` confirms:
- the library does not send bare `0x05`
- it sends `[0x05, packetSize]`
- for one-shot reads it expects a response shaped like `C0 00 05 ...payload`

- `datasheets.js` contains a `WirelessSoilMoisture` interface entry and a soil moisture sensor definition with:
- sensor ID `2065`
- raw measurement `adc`
- `adc` type `RawDigital`
- `DataSize = 2`

Important caveat:

- this packaged soil moisture definition is for the PASCO wireless soil moisture sensor family, not necessarily the exact PASPORT soil moisture sensor plugged into the AirLink
- but it still strongly supports the idea that the relevant raw reading is a 2-byte ADC-like value

## ESP32 probe results so far

Simple probe commands and PASCO-style command probes did not unlock a richer or faster measurement stream.

Commands already tested through the sketch:

- `probe`
- `probe0`
- `probe1`
- `pasco`
- `pasco0`
- `pasco1`
- `sample0`
- `sample1`

The result was:

- the recurring `0x85` stream remained the main observable live data path
- no clearly richer packet family appeared from the tested command set
- exact `READ_ONE_SAMPLE` tests using `05 02` also did not produce the expected `C0 00 05` response

Most recent important interpretation:

- the failed `sample0` / `sample1` tests do not necessarily mean `READ_ONE_SAMPLE` is unsupported
- they more likely mean the ESP32 sketch is still writing to the wrong PASCO characteristic
- based on `pasco-ble`, the next sketch patch should:
- write commands to `...0002`
- listen on `...0003`
- use `...0005` for acknowledgements if needed

## Raw-value baselines captured so far

### Air baseline

Stable air captures were roughly:

- `likelyRawADC`: `3963` to `3969`
- `stateByte`: `92` to `93`
- `flags`: `0,0`

### Water submerged baseline

Stable water-submerged captures were roughly:

- `likelyRawADC`: `3952` to `3961`
- `stateByte`: `90` to `92`
- `flags`: `0,0`

Current interpretation:

- air reads slightly higher than submerged water
- water reads slightly lower than air
- the response is real but much less dramatic than the SPARKvue graph, which implies the observed `0x85` frame is likely lower-level than the final SPARKvue presentation

## Most likely next step

If continuing from here, the next Codex should prioritize one of:

1. Patch the ESP32 sketch to follow the PASCO channel roles from `pasco-ble`:
   command on `...0002`, response on `...0003`, ack on `...0005`.
2. Retry `READ_ONE_SAMPLE` using `[0x05, 0x02]` after the channel-role fix and watch for a `C0 00 05` response.
3. If that still fails, continue treating `0x85` as the working raw ADC stream and compare it more systematically against SPARKvue `Voltage` and `%VWC`.

## Practical current conclusion

For the user's stated goal of getting as much of the raw sensor reading as possible, the current best answer is:

- use the `likelyRawADC` field decoded from the recurring `0x85` packet in `esp32PascoAirlink.ino`

That is the strongest raw-value candidate identified so far.
