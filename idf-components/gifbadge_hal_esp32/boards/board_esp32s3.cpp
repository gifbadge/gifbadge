#include <esp_app_desc.h>
#include <esp_pm.h>
#include <esp_mac.h>
#include <esp_ota_ops.h>
#include <sys/stat.h>
#include "boards/boards_esp32s3.h"
#include "log.h"
#include "esp_efuse_custom_table.h"
#include "esp_app_format.h"
#include "esp_ota.h"

static const char *TAG = "esp32::s3::esp32s3";

namespace Boards{

esp32::s3::esp32s3::esp32s3() {
  esp_pm_lock_create(ESP_PM_CPU_FREQ_MAX, 0, "Board Lock", &pmLockHandle);
  _config = new hal::config::esp32s3::Config_NVS();
}
void esp32::s3::esp32s3::Reset() {
  esp_restart();
}

void esp32::s3::esp32s3::PmLock() {
  esp_pm_lock_acquire(pmLockHandle);
}

void esp32::s3::esp32s3::PmRelease() {
  esp_pm_lock_release(pmLockHandle);
}

hal::config::Config *esp32::s3::esp32s3::GetConfig() {
  return _config;
}

void esp32::s3::esp32s3::DebugInfo() {
//  esp_pm_dump_locks(stdout);
//  heap_caps_print_heap_info(MALLOC_CAP_INTERNAL);
//  heap_caps_print_heap_info(MALLOC_CAP_SPIRAM);
}

const char *esp32::s3::esp32s3::SwVersion() {
  return esp_app_get_description()->version;
}
char *esp32::s3::esp32s3::SerialNumber() {
  uint8_t mac[6] = {0};
  esp_efuse_mac_get_default(mac);
  sprintf(serial, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
  return serial;
}
void esp32::s3::esp32s3::BootInfo() {
  const esp_partition_t *configured = esp_ota_get_boot_partition();
  const esp_partition_t *running = esp_ota_get_running_partition();

  LOGI(TAG, "Total Application Partitions: %d", esp_ota_get_app_partition_count());
  LOGI(TAG, "Running partition type %d subtype %d (offset 0x%08" PRIx32 ")",
       running->type, running->subtype, running->address);

  esp_app_desc_t boot_app_info;
  if (esp_ota_get_partition_description(configured, &boot_app_info) == ESP_OK) {
    LOGI(TAG, "Configured firmware version: %s", boot_app_info.version);
  }
  esp_app_desc_t running_app_info;
  if (esp_ota_get_partition_description(running, &running_app_info) == ESP_OK) {
    LOGI(TAG, "Running firmware version: %s", running_app_info.version);
  }
}
bool esp32::s3::esp32s3::OtaCheck() {
  struct stat buffer{};
  return (stat("/data/ota.bin", &buffer) == 0);
}
OtaError esp32::s3::esp32s3::OtaValidate() {
  FILE *ota_file = fopen("/data/ota.bin", "r");
  esp_image_header_t new_header_info;
  esp_app_desc_t new_app_info;
  esp_custom_app_desc_t new_custom_app_desc;
  fread(&new_header_info, 1, sizeof(esp_image_header_t), ota_file);
  fseek(ota_file, sizeof(esp_image_segment_header_t), SEEK_CUR);
  fread(&new_app_info, 1, sizeof(esp_app_desc_t), ota_file);
  fread(&new_custom_app_desc, 1, sizeof(new_custom_app_desc), ota_file);
  fclose(ota_file);

  LOGI(TAG, "New Firmware");
  LOGI(TAG, "CHIPID: %i", new_header_info.chip_id);
  LOGI(TAG, "Version: %s", new_app_info.version);
  LOGI(TAG, "Supports Boards:");
  for (int x = 0; x < new_custom_app_desc.num_supported_boards; x++) {
    LOGI(TAG, "%u", new_custom_app_desc.supported_boards[x]);
  }

  if (new_header_info.chip_id != ESP_CHIP_ID_ESP32S3) {
    return OtaError::WRONG_CHIP;
  }

  uint8_t board;
  esp_efuse_read_field_blob(ESP_EFUSE_USER_DATA_BOARD, &board, 8);
  bool supported_board = false;
  for (int x = 0; x < new_custom_app_desc.num_supported_boards; x++) {
    if (board == new_custom_app_desc.supported_boards[x]) {
      supported_board = true;
      break;
    }
  }
  if (!supported_board) {
    return OtaError::WRONG_BOARD;
  }

  const esp_partition_t *configured = esp_ota_get_boot_partition();
  const esp_partition_t *running = esp_ota_get_running_partition();

  if (configured != running) {
    ESP_LOGW(TAG, "Configured OTA boot partition at offset 0x%08" PRIx32", but running from offset 0x%08" PRIx32,
             configured->address, running->address);
    ESP_LOGW(TAG, "(This can happen if either the OTA boot data or preferred boot image become corrupted somehow.)");
  }
  LOGI(TAG, "Running partition type %d subtype %d (offset 0x%08" PRIx32")",
       running->type, running->subtype, running->address);
  esp_app_desc_t running_app_info;

  if (esp_ota_get_partition_description(running, &running_app_info) == ESP_OK) {
    LOGI(TAG, "Running firmware version: %s", running_app_info.version);
  }
//  if (memcmp(running_app_info.version, new_app_info.version, sizeof(new_app_info.version)) == 0) {
//    return OTA_SAME_VERSION;
//  }

  return OtaError::OK;
}
void esp32::s3::esp32s3::OtaInstall() {
  if (!_ota_task_handle) {
    xTaskCreate(OtaInstallTask, "task", 10000, this, 2, &_ota_task_handle);
  }
}
int esp32::s3::esp32s3::OtaStatus() {
  return _ota_status;
}

void esp32::s3::esp32s3::OtaInstallTask(void *arg) {
  esp_err_t err;
  auto *board = static_cast<esp32s3 *>(arg);

  size_t ota_size = 0;
  struct stat buffer{};
  if (stat("/data/ota.bin", &buffer) == 0) {
    ota_size = buffer.st_size;
  }
  LOGI(TAG, "OTA Update is %zu", ota_size);

  esp_ota_handle_t update_handle = 0;
  const esp_partition_t *update_partition;

  const esp_partition_t *configured = esp_ota_get_boot_partition();
  const esp_partition_t *running = esp_ota_get_running_partition();

  if (configured != running) {
    ESP_LOGW(TAG,
             "Configured OTA boot partition at offset 0x%08" PRIx32", but running from offset 0x%08" PRIx32,
             configured->address,
             running->address);
    ESP_LOGW(TAG, "(This can happen if either the OTA boot data or preferred boot image become corrupted somehow.)");
  }
  LOGI(TAG,
       "Running partition type %d subtype %d (offset 0x%08" PRIx32")",
       running->type,
       running->subtype,
       running->address);

  update_partition = esp_ota_get_next_update_partition(nullptr);
  assert(update_partition != nullptr);
  LOGI(TAG,
       "Writing to partition subtype %d at offset 0x%" PRIx32,
       update_partition->subtype,
       update_partition->address);

  Boards::OtaError validation_err = board->OtaValidate();
  if (validation_err != Boards::OtaError::OK) {
    ESP_LOGE(TAG, "validate failed with %d", (int) validation_err);
    vTaskDelete(nullptr);
  }

  err = esp_ota_begin(update_partition, ota_size, &update_handle);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "esp_ota_begin failed (%s)", esp_err_to_name(err));
    esp_ota_abort(update_handle);
    vTaskDelete(nullptr);
  }

#define OTA_BUFFER_SIZE (4096+1)

  FILE *ota_file = fopen("/data/ota.bin", "r");
  static char *ota_buffer = static_cast<char *>(malloc(OTA_BUFFER_SIZE));

  size_t bytes_read;

  board->_ota_status = 0;
  while ((bytes_read = fread(ota_buffer, 1, OTA_BUFFER_SIZE, ota_file)) > 0) {

    //Update the progress on the display
    int percent = static_cast<int>((100*ftell(ota_file) + ota_size/2)/ota_size);
    LOGI(TAG, "%%%d", percent);
    board->_ota_status = percent;

    err = esp_ota_write(update_handle, (const void *) ota_buffer, bytes_read);
    if (err != ESP_OK) {
      ESP_LOGE(TAG, "esp_ota_write failed (%s)!", esp_err_to_name(err));
      esp_ota_abort(update_handle);
    }
  }

  LOGI(TAG, "Writing Done. Deleting OTA File");
  fclose(ota_file);
  remove("/data/ota.bin");

  err = esp_ota_end(update_handle);
  if (err != ESP_OK) {
    if (err == ESP_ERR_OTA_VALIDATE_FAILED) {
      ESP_LOGE(TAG, "Image validation failed, image is corrupted");
    } else {
      ESP_LOGE(TAG, "esp_ota_end failed (%s)!", esp_err_to_name(err));
    }
    if(ota_buffer){
      free(ota_buffer);
    }
    vTaskDelete(nullptr);
  }

  err = esp_ota_set_boot_partition(update_partition);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "esp_ota_set_boot_partition failed (%s)!", esp_err_to_name(err));
    if(ota_buffer){
      free(ota_buffer);
    }
    vTaskDelete(nullptr);
  }
  LOGI(TAG, "Prepare to restart system!");
  board->Reset();
}

}
