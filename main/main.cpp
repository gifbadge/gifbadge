#include <filesystem>
#include <vector>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_pm.h"

#include "esp_err.h"
#include "esp_log.h"

#include <cstring>
#include <nvs_flash.h>
#include <nvs_handle.hpp>


#include "ui/lvgl.h"
#include "keys.h"
#include "touch.h"
#include "hal/hal_usb.h"
#include "display.h"
#include "config.h"

#include "ota.h"
#include "hal/i2c.h"

#include "hw_init.h"

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

//#define ARRAY_SIZE_OFFSET   5   //Increase this if print_real_time_stats returns ESP_ERR_INVALID_SIZE

///**
// * @brief   Function to print the CPU usage of tasks over a given duration.
// *
// * This function will measure and print the CPU usage of tasks over a specified
// * number of ticks (i.e. real time stats). This is implemented by simply calling
// * uxTaskGetSystemState() twice separated by a delay, then calculating the
// * differences of task run times before and after the delay.
// *
// * @note    If any tasks are added or removed during the delay, the stats of
// *          those tasks will not be printed.
// * @note    This function should be called from a high priority task to minimize
// *          inaccuracies with delays.
// * @note    When running in dual core mode, each core will correspond to 50% of
// *          the run time.
// *
// * @param   xTicksToWait    Period of stats measurement
// *
// * @return
// *  - ESP_OK                Success
// *  - ESP_ERR_NO_MEM        Insufficient memory to allocated internal arrays
// *  - ESP_ERR_INVALID_SIZE  Insufficient array size for uxTaskGetSystemState. Trying increasing ARRAY_SIZE_OFFSET
// *  - ESP_ERR_INVALID_STATE Delay duration too short
// */
//static esp_err_t print_real_time_stats(TickType_t xTicksToWait)
//{
//    TaskStatus_t *start_array = nullptr, *end_array = nullptr;
//    UBaseType_t start_array_size, end_array_size;
//    uint32_t start_run_time, end_run_time;
//    esp_err_t ret;
//
//    //Allocate array to store current task states
//    start_array_size = uxTaskGetNumberOfTasks() + ARRAY_SIZE_OFFSET;
//    start_array = static_cast<TaskStatus_t *>(malloc(sizeof(TaskStatus_t) * start_array_size));
//    if (start_array == nullptr) {
//        ret = ESP_ERR_NO_MEM;
//        free(start_array);
//        free(end_array);
//        return ret;
//    }
//    //Get current task states
//    start_array_size = uxTaskGetSystemState(start_array, start_array_size, &start_run_time);
//    if (start_array_size == 0) {
//        ret = ESP_ERR_INVALID_SIZE;
//        free(start_array);
//        free(end_array);
//        return ret;
//    }
//
//    vTaskDelay(xTicksToWait);
//
//    //Allocate array to store tasks states post delay
//    end_array_size = uxTaskGetNumberOfTasks() + ARRAY_SIZE_OFFSET;
//    end_array = static_cast<TaskStatus_t *>(malloc(sizeof(TaskStatus_t) * end_array_size));
//    if (end_array == nullptr) {
//        ret = ESP_ERR_NO_MEM;
//        free(start_array);
//        free(end_array);
//        return ret;
//    }
//    //Get post delay task states
//    end_array_size = uxTaskGetSystemState(end_array, end_array_size, &end_run_time);
//    if (end_array_size == 0) {
//        ret = ESP_ERR_INVALID_SIZE;
//        free(start_array);
//        free(end_array);
//        return ret;
//    }
//
//    //Calculate total_elapsed_time in units of run time stats clock period.
//    uint32_t total_elapsed_time = (end_run_time - start_run_time);
//    if (total_elapsed_time == 0) {
//        ret = ESP_ERR_INVALID_STATE;
//        free(start_array);
//        free(end_array);
//        return ret;
//    }
//
//    printf("| Task | Run Time | Percentage\n");
//    //Match each task in start_array to those in the end_array
//    for (int i = 0; i < start_array_size; i++) {
//        int k = -1;
//        for (int j = 0; j < end_array_size; j++) {
//            if (start_array[i].xHandle == end_array[j].xHandle) {
//                k = j;
//                //Mark that task have been matched by overwriting their handles
//                start_array[i].xHandle = nullptr;
//                end_array[j].xHandle = nullptr;
//                break;
//            }
//        }
//        //Check if matching task found
//        if (k >= 0) {
//            uint32_t task_elapsed_time = end_array[k].ulRunTimeCounter - start_array[i].ulRunTimeCounter;
//            uint32_t percentage_time = (task_elapsed_time * 100UL) / (total_elapsed_time * portNUM_PROCESSORS);
//            printf("| %s | %" PRIu32" | %" PRIu32"%%\n", start_array[i].pcTaskName, task_elapsed_time, percentage_time);
//        }
//    }
//
//    //Print unmatched tasks
//    for (int i = 0; i < start_array_size; i++) {
//        if (start_array[i].xHandle != nullptr) {
//            printf("| %s | Deleted\n", start_array[i].pcTaskName);
//        }
//    }
//    for (int i = 0; i < end_array_size; i++) {
//        if (end_array[i].xHandle != nullptr) {
//            printf("| %s | Created\n", end_array[i].pcTaskName);
//        }
//    }
//    ret = ESP_OK;
//    free(start_array);
//    free(end_array);
//    return ret;
//}

void dump_state(void *arg) {
    auto *args = (sharedState *) arg;
    esp_pm_lock_handle_t pm_lock;
    esp_pm_lock_create(ESP_PM_NO_LIGHT_SLEEP, 0, "output_lock", &pm_lock);

    while (true) {
        esp_pm_lock_acquire(pm_lock);
//        vTaskDelay(1000/portTICK_PERIOD_MS);
//        if (true) {
//            esp_pm_dump_locks(stdout);
//        }
        ESP_LOGI(TAG, "SOC: %i", args->board->getBattery()->getSoc());
        ESP_LOGI(TAG, "Voltage: %f", args->board->getBattery()->getVoltage());
        ESP_LOGI(TAG, "Rate: %f", args->board->getBattery()->getRate());
        ESP_LOGI(TAG, "%s", args->board->getBattery()->isCharging()?"Charging":"Discharging");


//        printf("\n\nGetting real time stats over %" PRIu32" ticks\n", pdMS_TO_TICKS(1000));
//        if (print_real_time_stats(pdMS_TO_TICKS(1000)) == ESP_OK) {
//            printf("Real time stats obtained\n");
//        } else {
//            printf("Error getting real time stats\n");
//        }
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

    xTaskCreate(display_task, "display_task", 20000, &args, 2, &display_task_handle);

    sharedState configState{
            imageconfig,
            board,
    };

    xTaskCreate(dump_state, "dump_state", 20000, &configState, 2, nullptr);

    ota_boot_info();
    ota_init();

    lvgl_init(board, imageconfig);

    vTaskDelay(1000 / portTICK_PERIOD_MS); //Let USB Settle

    if(!board->storageReady()){
        xTaskNotifyIndexed(display_task_handle, 0, DISPLAY_NO_STORAGE, eSetValueWithOverwrite);
        return;
    }

    input_event i{};
    MAIN_STATES oldState = MAIN_NONE;
    int64_t last_change = esp_timer_get_time();
    while (true) {
        if (oldState != currentState) {
            ESP_LOGI(TAG, "State %d", currentState);
            switch (currentState) {
                case MAIN_NONE:
                    break;
                case MAIN_NORMAL:
                    if (oldState == MAIN_USB) {
                        //Check for OTA File
                        if (ota_check()) {
                            currentState = MAIN_OTA;
                            xTaskNotifyIndexed(display_task_handle, 0, DISPLAY_OTA, eSetValueWithOverwrite);
                            break;
                        }
                    }
//                    vTaskDelay(500 / portTICK_PERIOD_MS);
                    xTaskNotifyIndexed(display_task_handle, 0, DISPLAY_FILE, eSetValueWithOverwrite);
                    break;
                case MAIN_USB:
                    xTaskNotifyIndexed(display_task_handle, 0, DISPLAY_USB, eSetValueWithOverwrite);
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
                    xTaskNotifyIndexed(display_task_handle, 0, DISPLAY_PREVIOUS, eSetValueWithOverwrite);
                }
                if (i.code == KEY_DOWN && i.value == STATE_PRESSED) {
                    xQueueReceive(input_queue, (void *) &i, 0);
                    xTaskNotifyIndexed(display_task_handle, 0, DISPLAY_NEXT, eSetValueWithOverwrite);
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
            TaskHandle_t lvglHandle = xTaskGetHandle("LVGL");
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
