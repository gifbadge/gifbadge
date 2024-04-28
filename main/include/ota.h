#pragma once

typedef struct {
  uint8_t num_supported_boards;
  uint8_t supported_boards[];
} esp_custom_app_desc_t;

#ifdef __cplusplus
namespace OTA {

/*!
 * OTA Validation return value
 */
enum class validation_err {
  OK = 0,
  WRONG_CHIP,
  WRONG_BOARD,
  SAME_VERSION,

};

void bootInfo();
bool check();
void install();

validation_err validate();
int ota_status();
}
#endif

