/*******************************************************************************
 * Copyright (c) 2026 GifBadge
 *
 * SPDX-License-Identifier:   GPL-3.0-or-later
 ******************************************************************************/

#pragma once

#include <hal/board.h>
#include <esp_pm.h>
#include "drivers/config_nvs.h"
#include "tinyusb_msc.h"

namespace Boards::esp32::p4 {
class esp32p4 : public Board {
  public:
    size_t MemorySize() override;

    esp32p4();
    ~esp32p4() override = default;

    void PmLock() override;
    void PmRelease() override;

    hal::config::Config *GetConfig() override;
    void DebugInfo() override;
    const char *SwVersion() override;
    void Reset() override;
    char *SerialNumber() override;
    void BootInfo() override;
    bool OtaCheck() override;
    static OtaError OtaHeaderValidate(uint8_t const *data);
    OtaError OtaValidate() override;
    void OtaInstall() override;
    int OtaStatus() override;

  protected:
    esp_pm_lock_handle_t pmLockHandle = nullptr;
    hal::config::esp32s3::Config_NVS *_config;
    char serial[18] = {0};
    tinyusb_msc_storage_handle_t storage_handle = nullptr;
    char storage_base_path[6] = "/data";

    static void OtaInstallTask(void *arg);
    TaskHandle_t _ota_task_handle = nullptr;
    int _ota_status = -1;
    esp_err_t esp32p4_usb_init(gpio_num_t usb_sense);
};
}

const char *lltoa(long long val, int base);
