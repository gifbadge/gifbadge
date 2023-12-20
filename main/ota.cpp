#include <esp_partition.h>
#include <esp_ota_ops.h>
#include <sys/stat.h>
#include <cstring>
#include <esp_app_format.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "ota.h"

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

#define BUFFSIZE 1024
static char ota_write_data[BUFFSIZE + 1] = { 0 };

void ota_task(void *){
    vTaskSuspend(nullptr); //Wait until we are actually needed

    esp_err_t err;

    size_t ota_size = 0;
    struct stat   buffer{};
    if(stat ("/data/ota.bin", &buffer) == 0){
        ota_size = buffer.st_size;
    }
    ESP_LOGI(TAG, "OTA Update is %zu", ota_size);

    FILE *ota_file = fopen("/data/ota.bin", "r");
    esp_ota_handle_t update_handle = 0 ;
    const esp_partition_t *update_partition;

    ESP_LOGI(TAG, "Starting OTA example task");

    const esp_partition_t *configured = esp_ota_get_boot_partition();
    const esp_partition_t *running = esp_ota_get_running_partition();

    if (configured != running) {
        ESP_LOGW(TAG, "Configured OTA boot partition at offset 0x%08" PRIx32", but running from offset 0x%08" PRIx32,
                 configured->address, running->address);
        ESP_LOGW(TAG, "(This can happen if either the OTA boot data or preferred boot image become corrupted somehow.)");
    }
    ESP_LOGI(TAG, "Running partition type %d subtype %d (offset 0x%08" PRIx32")",
             running->type, running->subtype, running->address);

    update_partition = esp_ota_get_next_update_partition(nullptr);
    assert(update_partition != nullptr);
    ESP_LOGI(TAG, "Writing to partition subtype %d at offset 0x%" PRIx32,
             update_partition->subtype, update_partition->address);

    int binary_file_length = 0;
    /*deal with all receive packet*/
    bool image_header_was_checked = false;
    while (true) {
        int data_read = fread(ota_write_data, 1, BUFFSIZE, ota_file);
//        ESP_LOGI(TAG, "%zu, %i", ota_size, binary_file_length);


//        //Update the progress on the display
        TaskHandle_t display_task_handle = xTaskGetHandle("display_task");
        uint32_t percent = ((float)ftell(ota_file)/ota_size)*100;
        ESP_LOGI(TAG, "%%%lu", percent);
        xTaskNotifyIndexed(display_task_handle, 1, percent, eSetValueWithOverwrite);

        if (data_read > 0) {
            if (!image_header_was_checked) {
                esp_app_desc_t new_app_info;
                if (data_read > sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t) + sizeof(esp_app_desc_t)) {
                    // check current version with downloading
                    memcpy(&new_app_info, &ota_write_data[sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t)], sizeof(esp_app_desc_t));
                    ESP_LOGI(TAG, "New firmware version: %s", new_app_info.version);

                    esp_app_desc_t running_app_info;
                    if (esp_ota_get_partition_description(running, &running_app_info) == ESP_OK) {
                        ESP_LOGI(TAG, "Running firmware version: %s", running_app_info.version);
                    }

                    const esp_partition_t* last_invalid_app = esp_ota_get_last_invalid_partition();
                    esp_app_desc_t invalid_app_info;
                    if (esp_ota_get_partition_description(last_invalid_app, &invalid_app_info) == ESP_OK) {
                        ESP_LOGI(TAG, "Last invalid firmware version: %s", invalid_app_info.version);
                    }

//                    // check current version with last invalid partition
//                    if (last_invalid_app != nullptr) {
//                        if (memcmp(invalid_app_info.version, new_app_info.version, sizeof(new_app_info.version)) == 0) {
//                            ESP_LOGW(TAG, "New version is the same as invalid version.");
//                            ESP_LOGW(TAG, "Previously, there was an attempt to launch the firmware with %s version, but it failed.", invalid_app_info.version);
//                            ESP_LOGW(TAG, "The firmware has been rolled back to the previous version.");
//                            http_cleanup(client);
//                            infinite_loop();
//                        }
//                    }
                    image_header_was_checked = true;

                    err = esp_ota_begin(update_partition, OTA_WITH_SEQUENTIAL_WRITES, &update_handle);
                    if (err != ESP_OK) {
                        ESP_LOGE(TAG, "esp_ota_begin failed (%s)", esp_err_to_name(err));
                        esp_ota_abort(update_handle);
                    }
                    ESP_LOGI(TAG, "esp_ota_begin succeeded");
                } else {
                    ESP_LOGE(TAG, "received package is not fit len");
                }
            }
            err = esp_ota_write( update_handle, (const void *)ota_write_data, data_read);
            if (err != ESP_OK) {
                esp_ota_abort(update_handle);
            }
            binary_file_length += data_read;
            ESP_LOGD(TAG, "Written image length %d", binary_file_length);
        } else {
            break;
        }
    }
    ESP_LOGI(TAG, "Total Write binary data length: %d", binary_file_length);

    ESP_LOGI(TAG, "Deleting OTA File");
    fclose(ota_file);
    remove("/data/ota.bin");

    err = esp_ota_end(update_handle);
    if (err != ESP_OK) {
        if (err == ESP_ERR_OTA_VALIDATE_FAILED) {
            ESP_LOGE(TAG, "Image validation failed, image is corrupted");
        } else {
            ESP_LOGE(TAG, "esp_ota_end failed (%s)!", esp_err_to_name(err));
        }
    }

    err = esp_ota_set_boot_partition(update_partition);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_set_boot_partition failed (%s)!", esp_err_to_name(err));
    }
    ESP_LOGI(TAG, "Prepare to restart system!");
    esp_restart();
}

TaskHandle_t ota_task_handle;

void ota_install(){
    vTaskResume(ota_task_handle);
}

void ota_init(){
    xTaskCreate(ota_task, "ota_task", 20000, nullptr, 2, &ota_task_handle);
}