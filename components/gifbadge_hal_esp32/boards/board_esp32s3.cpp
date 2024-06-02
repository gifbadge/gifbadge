#include <esp_app_desc.h>
#include <esp_pm.h>
#include "boards/boards_esp32s3.h"

namespace Boards{

esp32s3::esp32s3() {
  esp_pm_lock_create(ESP_PM_CPU_FREQ_MAX, 0, "Board Lock", &pmLockHandle);
  _config = new Config_NVS();
}

void esp32s3::pmLock() {
  esp_pm_lock_acquire(pmLockHandle);
}

void esp32s3::pmRelease() {
  esp_pm_lock_release(pmLockHandle);
}

Config *esp32s3::getConfig() {
  return _config;
}

void esp32s3::debugInfo() {
  esp_pm_dump_locks(stdout);
  heap_caps_print_heap_info(MALLOC_CAP_INTERNAL);
  heap_caps_print_heap_info(MALLOC_CAP_SPIRAM);
}

const char *esp32s3::swVersion() {
  return esp_app_get_description()->version;
}
}
