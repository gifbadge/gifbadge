#pragma once

typedef struct {
  uint8_t num_supported_boards;
  uint8_t supported_boards[];
} esp_custom_app_desc_t;

typedef enum {
  OTA_OK = 0,
  OTA_WRONG_CHIP,
  OTA_WRONG_BOARD,
  OTA_SAME_VERSION,

} ota_validation_err;

#ifdef __cplusplus
void ota_boot_info();
bool ota_check();
void ota_install();
void ota_init();

ota_validation_err ota_validate();
#endif

