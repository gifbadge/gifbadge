#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_pm.h"

#include "esp_err.h"
#include "esp_log.h"

#include <cstring>
#include <nvs_flash.h>
#include <nvs_handle.hpp>


#include "ui/menu.h"
#include "keys.h"
#include "hal/hal_usb.h"
#include "display.h"
#include "config.h"

#include "ota.h"
#include "hal/i2c.h"

#include "hw_init.h"
#include "ui/usb_connected.h"

static const char *TAG = "MAIN";



struct sharedState {
    std::shared_ptr<ImageConfig> image_config;
    std::shared_ptr<Board> board;
};

enum MAIN_STATES {
    MAIN_NONE,
    MAIN_NORMAL,
    MAIN_USB,
    MAIN_LOW_BATT,
    MAIN_OTA,
};

void dump_state(void *arg) {
    auto *args = (sharedState *) arg;
    esp_pm_lock_handle_t pm_lock;
    esp_pm_lock_create(ESP_PM_NO_LIGHT_SLEEP, 0, "output_lock", &pm_lock);

    while (true) {
        esp_pm_lock_acquire(pm_lock);
//        vTaskDelay(1000/portTICK_PERIOD_MS);
        if (true) {
            esp_pm_dump_locks(stdout);
            char out[1000];
          vTaskGetRunTimeStats(out);
          printf("%s", out);
        }
        ESP_LOGI(TAG, "SOC: %i", args->board->getBattery()->getSoc());
        ESP_LOGI(TAG, "Voltage: %f", args->board->getBattery()->getVoltage());
        ESP_LOGI(TAG, "Rate: %f", args->board->getBattery()->getRate());
        ESP_LOGI(TAG, "%s", args->board->getBattery()->isCharging()?"Charging":"Discharging");


        esp_pm_lock_release(pm_lock);
        vTaskDelay(10000 / portTICK_PERIOD_MS);
    }
}

MAIN_STATES currentState = MAIN_NORMAL;

extern "C" void app_main(void) {
    esp_err_t err;


    err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    storage_callback([](bool state) {
        ESP_LOGI(TAG, "state %u", state);
        if (state) {
            currentState = MAIN_NORMAL;
        } else {
            currentState = MAIN_USB;
        }
    });


    std::shared_ptr<Board> board = get_board();

    auto imageconfig = std::make_shared<ImageConfig>();


    QueueHandle_t input_queue = xQueueCreate(10, sizeof(input_event));
    input_init(board->getKeys(), input_queue);


    TaskHandle_t display_task_handle = nullptr;

    display_task_args args = {
            board->getDisplay(),
            imageconfig,
            board->getBacklight(),
    };

    xTaskCreate(display_task, "display_task", 10000, &args, 2, &display_task_handle);

    sharedState configState{
            imageconfig,
            board,
    };

    xTaskCreate(dump_state, "dump_state", 10000, &configState, 2, nullptr);

    ota_boot_info();

    lvgl_init(board);

    vTaskDelay(1000 / portTICK_PERIOD_MS); //Let USB Settle

    if(!board->storageReady()){
        xTaskNotifyIndexed(display_task_handle, 0, DISPLAY_NO_STORAGE, eSetValueWithOverwrite);
        return;
    }

    input_event i{};
    MAIN_STATES oldState = MAIN_NONE;
    int64_t last_change = esp_timer_get_time();
    TaskHandle_t lvglHandle = xTaskGetHandle("LVGL");
  while (true) {
        if (oldState != currentState) {
            ESP_LOGI(TAG, "State %d", currentState);
            switch (currentState) {
                case MAIN_NONE:
                    break;
                case MAIN_NORMAL:
                    if (oldState == MAIN_USB) {
                      xTaskNotifyIndexed(lvglHandle, 0, LVGL_STOP, eSetValueWithOverwrite);
                      //Check for OTA File
                        if (ota_check()) {
                            ota_install();
                            currentState = MAIN_OTA;
                            xTaskNotifyIndexed(display_task_handle, 0, DISPLAY_OTA, eSetValueWithOverwrite);
                            break;
                        }
                    }
                    xTaskNotifyIndexed(display_task_handle, 0, DISPLAY_FILE, eSetValueWithOverwrite);
                    break;
                case MAIN_USB:
                  xTaskNotifyIndexed(display_task_handle, 0, DISPLAY_MENU, eSetValueWithOverwrite);
                  vTaskDelay(100 / portTICK_PERIOD_MS);
                  lvgl_wake_up();
                  lvgl_usb_connected();
                    break;
                case MAIN_LOW_BATT:
                    xTaskNotifyIndexed(display_task_handle, 0, DISPLAY_BATT, eSetValueWithOverwrite);
                    break;
                case MAIN_OTA:
                    break;
            }
            oldState = currentState;
        } else if (currentState == MAIN_NORMAL) {
            if (xQueuePeek(input_queue, (void *) &i, 50 / portTICK_PERIOD_MS) && !lvgl_menu_state()) {
                get_event(i);
                if (i.code == KEY_ENTER && i.value == STATE_PRESSED) {
                    xQueueReceive(input_queue, (void *) &i, 0);
                    xTaskNotifyIndexed(display_task_handle, 0, DISPLAY_MENU, eSetValueWithOverwrite);
                    vTaskDelay(100 / portTICK_PERIOD_MS);
                    lvgl_menu_open();
                }
                if (i.code == KEY_UP && i.value == STATE_PRESSED) {
                    xQueueReceive(input_queue, (void *) &i, 0);
                    xTaskNotifyIndexed(display_task_handle, 0, DISPLAY_NEXT, eSetValueWithOverwrite);
                }
                if (i.code == KEY_DOWN && i.value == STATE_PRESSED) {
                    xQueueReceive(input_queue, (void *) &i, 0);
                    xTaskNotifyIndexed(display_task_handle, 0, DISPLAY_PREVIOUS, eSetValueWithOverwrite);
                }
                if ((esp_timer_get_time() - i.timestamp) / 1000 > 200) {
                    xQueueReceive(input_queue, (void *) &i, 0);
                }
            }
            if (lvgl_menu_state()) {
                //Eat input events when we can't use them
                xQueueReset(input_queue);
            }
            if (!lvgl_menu_state() && board->getTouch()) {
                auto e = board->getTouch()->read();
                if(e.first > 0 && e.second > 0){
                    if (((esp_timer_get_time() / 1000) - (last_change / 1000)) > 1000) {
                        last_change = esp_timer_get_time();
                        if (e.second < 50) {
                            xTaskNotifyIndexed(display_task_handle, 0, DISPLAY_MENU, eSetValueWithOverwrite);
                            vTaskDelay(100 / portTICK_PERIOD_MS);
                            lvgl_menu_open();
                        }
                        if (e.first < 50) {
                            xTaskNotifyIndexed(display_task_handle, 0, DISPLAY_PREVIOUS, eSetValueWithOverwrite);
                        }
                        if (e.first > 430) {
                            xTaskNotifyIndexed(display_task_handle, 0, DISPLAY_NEXT, eSetValueWithOverwrite);
                        }
                    }
                    ESP_LOGI(TAG, "x: %d y: %d", e.first, e.second);
                }
            }
        } else {
            //Eat input events when we can't use them
            xQueueReset(input_queue);
        }
        if (currentState != MAIN_USB) {
            switch(board->powerState()){
                case BOARD_POWER_NORMAL:
                    if(currentState == MAIN_LOW_BATT){
                        currentState = MAIN_NORMAL;
                    }
                    break;
                case BOARD_POWER_LOW:
                    xTaskNotifyIndexed(lvglHandle, 0, LVGL_STOP, eSetValueWithOverwrite);
                    currentState = MAIN_LOW_BATT;
                    break;
                case BOARD_POWER_CRITICAL:
                    xTaskNotifyIndexed(display_task_handle, 0, DISPLAY_BATT, eSetValueWithOverwrite);
                    vTaskDelay(15000/portTICK_PERIOD_MS);
                    board->powerOff();
                    break;
            }
        }
        if (currentState == MAIN_OTA) {
            ota_install();
        }


        vTaskDelay(100 / portTICK_PERIOD_MS);
    }


}
