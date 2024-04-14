#pragma once

typedef struct {
  uint8_t num_supported_boards;
  uint8_t supported_boards[];
} esp_custom_app_desc_t;

#ifdef __cplusplus
namespace OTA {

typedef enum {
  OTA_OK = 0,
  OTA_WRONG_CHIP,
  OTA_WRONG_BOARD,
  OTA_SAME_VERSION,

} ota_validation_err;

void bootInfo();
bool check();
void install();

ota_validation_err validate();
}
#endif

