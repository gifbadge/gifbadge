# _GifBadge_Software_
This repository contains the core software for the GifBadge devices.

It can be built to target both ESP32-S3 and Linux with SDL3 currently.

This project proudly contains no AI generated code. All bugs created by humans and other creatures.
Contributions from AI are __not__ welcome.

## Building
### ESP32-S3
Ensure you have a working esp-idf installation. For instructions see [here](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/get-started/index.html#installation)

Apply the correct configuration for your device

#### 1.28"
`SDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.defaults.esp32s3;sdkconfig.defaults.1_28" idf.py set-target esp32s3`

#### 2.1"
`SDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.defaults.esp32s3;sdkconfig.defaults.2_1" idf.py set-target esp32s3`

Compile with `idf.py build`

Flash with `idf.py flash`

### Linux
Requires SDL3, cmake

Build with
`cmake . -B build`
