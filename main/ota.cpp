#include <esp_partition.h>
#include <esp_ota_ops.h>
#include <sys/stat.h>
#include <cstring>
#include <esp_app_format.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include <cinttypes>

#include "ota.h"
#include "esp_efuse_custom_table.h"

static const char *TAG = "OTA";

void ota_boot_info() {
    const esp_partition_t *configured = esp_ota_get_boot_partition();
    const esp_partition_t *running = esp_ota_get_running_partition();

    ESP_LOGI(TAG, "Total Application Partitions: %d", esp_ota_get_app_partition_count());
    ESP_LOGI(TAG, "Running partition type %d subtype %d (offset 0x%08" PRIx32 ")",
             running->type, running->subtype, running->address);

    esp_app_desc_t boot_app_info;
    if (esp_ota_get_partition_description(configured, &boot_app_info) == ESP_OK) {
        ESP_LOGI(TAG, "Configured firmware version: %s", boot_app_info.version);
    }
    esp_app_desc_t running_app_info;
    if (esp_ota_get_partition_description(running, &running_app_info) == ESP_OK) {
        ESP_LOGI(TAG, "Running firmware version: %s", running_app_info.version);
    }
}

bool ota_check(){
    struct stat   buffer{};
    return (stat ("/data/ota.bin", &buffer) == 0);
}

ota_validation_err ota_validate() {
  FILE *ota_file = fopen("/data/ota.bin", "r");
  esp_image_header_t new_header_info;
  esp_app_desc_t new_app_info;
  esp_custom_app_desc_t new_custom_app_desc;
  fread(&new_header_info, 1, sizeof(esp_image_header_t), ota_file);
  fseek(ota_file, sizeof(esp_image_segment_header_t), SEEK_CUR);
  fread(&new_app_info, 1, sizeof(esp_app_desc_t), ota_file);
  fread(&new_custom_app_desc, 1, sizeof(new_custom_app_desc), ota_file);
  fseek(ota_file, sizeof(esp_image_header_t)+sizeof(esp_image_segment_header_t)+sizeof(esp_app_desc_t)+1, SEEK_SET);
  auto *supported_boards = static_cast<uint8_t *>(malloc(new_custom_app_desc.num_supported_boards));
  fread(supported_boards, 1,new_custom_app_desc.num_supported_boards, ota_file);
  fclose(ota_file);

  ESP_LOGI(TAG, "New Firmware");
  ESP_LOGI(TAG, "CHIPID: %" PRIu16 "", new_header_info.chip_id);
  ESP_LOGI(TAG, "Version: %s", new_app_info.version);
  ESP_LOGI(TAG, "Supports Boards:");
  for (int x = 0; x < new_custom_app_desc.num_supported_boards; x++) {
    ESP_LOGI(TAG, "%" PRIu8 "", supported_boards[x]);
  }

  if (new_header_info.chip_id!=ESP_CHIP_ID_ESP32S3) {
    return OTA_WRONG_CHIP;
  }

  uint8_t board;
  esp_efuse_read_field_blob(ESP_EFUSE_USER_DATA_BOARD, &board, 8);
  bool supported_board = false;
  for (int x = 0; x < new_custom_app_desc.num_supported_boards; x++) {
    if(board == supported_boards[x]){
      supported_board = true;
      break;
    }
  }
  if(!supported_board){
    return OTA_WRONG_BOARD;
  }

  const esp_partition_t *configured = esp_ota_get_boot_partition();
  const esp_partition_t *running = esp_ota_get_running_partition();

  if (configured != running) {
    ESP_LOGW(TAG, "Configured OTA boot partition at offset 0x%08" PRIx32", but running from offset 0x%08" PRIx32,
             configured->address, running->address);
    ESP_LOGW(TAG, "(This can happen if either the OTA boot data or preferred boot image become corrupted somehow.)");
  }
  ESP_LOGI(TAG, "Running partition type %d subtype %d (offset 0x%08" PRIx32")",
           running->type, running->subtype, running->address);
  esp_app_desc_t running_app_info;

  if (esp_ota_get_partition_description(running, &running_app_info) == ESP_OK) {
    ESP_LOGI(TAG, "Running firmware version: %s", running_app_info.version);
  }
//  if (memcmp(running_app_info.version, new_app_info.version, sizeof(new_app_info.version)) == 0) {
//    return OTA_SAME_VERSION;
//  }

  return OTA_OK;
}


void ota_task(void *) {
  esp_err_t err;

  size_t ota_size = 0;
  struct stat buffer{};
  if (stat("/data/ota.bin", &buffer)==0) {
    ota_size = buffer.st_size;
  }
  ESP_LOGI(TAG, "OTA Update is %zu", ota_size);


  esp_ota_handle_t update_handle = 0;
  const esp_partition_t *update_partition;

  const esp_partition_t *configured = esp_ota_get_boot_partition();
  const esp_partition_t *running = esp_ota_get_running_partition();

  if (configured!=running) {
    ESP_LOGW(TAG, "Configured OTA boot partition at offset 0x%08" PRIx32", but running from offset 0x%08" PRIx32, configured->address, running->address);
    ESP_LOGW(TAG, "(This can happen if either the OTA boot data or preferred boot image become corrupted somehow.)");
  }
  ESP_LOGI(TAG, "Running partition type %d subtype %d (offset 0x%08" PRIx32")", running->type, running->subtype, running->address);

  update_partition = esp_ota_get_next_update_partition(nullptr);
  assert(update_partition!=nullptr);
  ESP_LOGI(TAG, "Writing to partition subtype %d at offset 0x%" PRIx32, update_partition->subtype, update_partition->address);

  ota_validation_err validation_err = ota_validate();
  if(validation_err != OTA_OK){
    ESP_LOGE(TAG, "ota_validate failed with %i", validation_err);
    vTaskDelete(nullptr);
  }

  err = esp_ota_begin(update_partition, ota_size, &update_handle);
  if (err!=ESP_OK) {
    ESP_LOGE(TAG, "esp_ota_begin failed (%s)", esp_err_to_name(err));
    esp_ota_abort(update_handle);
    vTaskDelete(nullptr);
  }

  FILE *ota_file = fopen("/data/ota.bin", "r");
  static char ota_buffer[4096 + 1] = {0 };

  size_t bytes_read;
  while ((bytes_read = fread(ota_buffer, 1, sizeof(ota_buffer), ota_file)) > 0) {

    //Update the progress on the display
    TaskHandle_t display_task_handle = xTaskGetHandle("display_task");
    uint32_t percent = ((float) ftell(ota_file)/ota_size)*100;
    ESP_LOGI(TAG, "%%%lu", percent);
    xTaskNotifyIndexed(display_task_handle, 1, percent, eSetValueWithOverwrite);

    err = esp_ota_write(update_handle, (const void *) ota_buffer, bytes_read);
    if (err!=ESP_OK) {
      ESP_LOGE(TAG, "esp_ota_write failed (%s)!", esp_err_to_name(err));
      esp_ota_abort(update_handle);
    }
  }

  ESP_LOGI(TAG, "Writing Done. Deleting OTA File");
  fclose(ota_file);
  remove("/data/ota.bin");

  err = esp_ota_end(update_handle);
  if (err!=ESP_OK) {
    if (err==ESP_ERR_OTA_VALIDATE_FAILED) {
      ESP_LOGE(TAG, "Image validation failed, image is corrupted");
    } else {
      ESP_LOGE(TAG, "esp_ota_end failed (%s)!", esp_err_to_name(err));
    }
    vTaskDelete(nullptr);
  }

  err = esp_ota_set_boot_partition(update_partition);
  if (err!=ESP_OK) {
    ESP_LOGE(TAG, "esp_ota_set_boot_partition failed (%s)!", esp_err_to_name(err));
    vTaskDelete(nullptr);
  }
  ESP_LOGI(TAG, "Prepare to restart system!");
  esp_restart();
}

TaskHandle_t ota_task_handle;

void ota_install(){
  if(!ota_task_handle) {
    xTaskCreate(ota_task, "ota_task", 20000, nullptr, 2, &ota_task_handle);
  }
}

void ota_init(){
    xTaskCreate(ota_task, "ota_task", 20000, nullptr, 2, &ota_task_handle);
}