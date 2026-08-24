# AI Camera V1.1 Full-Feature Project

This directory contains only the files required to compile and upload the product's full-feature firmware. It does not include host software, factory test firmware,
schematics, old build caches, or historical archives.

## Directory Structure

- `AI_Camera-V1.1-Arduino`: A standalone, compilable Arduino main project containing the main program,
  `app`, `src`, and the project dependency `libraries`.
- `Factory_Firmware`: Precompiled full-feature firmware and a quick flashing script.
- `compile_full_firmware.bat`: Used by developers to recompile the full-feature firmware.

Open the following entry point in the Arduino IDE:

`AI_Camera-V1.1-Arduino\AI_Camera-V1.1-Arduino.ino`

## Fixed Build Parameters

- Board: ESP32S3 Dev Module
- ESP32 Arduino: 3.2.0-cn
- Flash: 16 MB
- Flash Mode: QIO
- PSRAM: OPI
- Partition Scheme: `elecrow_s3` (10 MB APP with OTA)
- USB CDC On Boot: Disabled/default
- Upload Speed: 921600

To update the firmware, double-click `compile_full_firmware.bat`. Upon successful completion, it will automatically refresh
the precompiled files in `Factory_Firmware`.