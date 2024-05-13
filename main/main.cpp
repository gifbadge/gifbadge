#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_pm.h"

#include "esp_err.h"
#include "esp_log.h"

#include <cstring>
#include <nvs_flash.h>
#include <nvs_handle.hpp>

#include "ui/menu.h"
#include "hal/hal_usb.h"
#include "display.h"

#include "ota.h"

#include "hw_init.h"
#include "ui/usb_connected.h"
#include "input.h"

static const char *TAG = "MAIN";

void dumpDebugFunc(void *arg) {
  auto *args = (Board *) arg;
  esp_pm_lock_handle_t pm_lock;
  esp_pm_lock_create(ESP_PM_NO_LIGHT_SLEEP, 0, "output_lock", &pm_lock);

    esp_pm_lock_acquire(pm_lock);
//        vTaskDelay(1000/portTICK_PERIOD_MS);
    if (false) {
      esp_pm_dump_locks(stdout);
      char out[1000];
      vTaskGetRunTimeStats(out);
      printf("%s", out);
    }
    ESP_LOGI(TAG, "SOC: %i", args->getBattery()->getSoc());
    ESP_LOGI(TAG, "Voltage: %f", args->getBattery()->getVoltage());
//    ESP_LOGI(TAG, "Rate: %f", args->getBattery()->getRate());
//    ESP_LOGI(TAG, "State: %d", static_cast<int>(args->getBattery()->status()));
    heap_caps_print_heap_info(MALLOC_CAP_INTERNAL);

    heap_caps_print_heap_info(MALLOC_CAP_SPIRAM);

    esp_pm_lock_release(pm_lock);
}

static void dumpDebugTimerInit(void *args) {
  const esp_timer_create_args_t dumpDebugTimerArgs = {
      .callback = &dumpDebugFunc,
      .arg = args,
      .dispatch_method = ESP_TIMER_TASK,
      .name = "dumpDebugTimer",
      .skip_unhandled_events = true
  };
  esp_timer_handle_t dumpDebugTimer = nullptr;
  ESP_ERROR_CHECK(esp_timer_create(&dumpDebugTimerArgs, &dumpDebugTimer));
  ESP_ERROR_CHECK(esp_timer_start_periodic(dumpDebugTimer, 10000 * 1000));
}

MAIN_STATES currentState = MAIN_NONE;

/*!
 * Task to check battery status, and update device
 * @param args
 */
static void lowBatteryTask(void *args) {
  Board *board = get_board();
  TaskHandle_t lvglHandle;
  TaskHandle_t display_task_handle;

  if (currentState != MAIN_OTA) {
    switch (board->powerState()) {
      case BOARD_POWER_NORMAL:
        if (currentState == MAIN_LOW_BATT) {
          currentState = MAIN_NORMAL;
        }
        break;
      case BOARD_POWER_LOW:
        lvglHandle = xTaskGetHandle("LVGL");
        xTaskNotifyIndexed(lvglHandle, 0, LVGL_STOP, eSetValueWithOverwrite);
        currentState = MAIN_LOW_BATT;
        break;
      case BOARD_POWER_CRITICAL:
        display_task_handle = xTaskGetHandle("display_task");
        xTaskNotifyIndexed(display_task_handle, 0, DISPLAY_BATT, eSetValueWithOverwrite);
        vTaskDelay(15000 / portTICK_PERIOD_MS);
        board->powerOff();
        break;
    }
  }
}

static void initLowBatteryTask() {
  const esp_timer_create_args_t lowBatteryArgs = {
      .callback = &lowBatteryTask,
      .arg = nullptr,
      .dispatch_method = ESP_TIMER_TASK,
      .name = "low_battery_handler",
      .skip_unhandled_events = true
  };
  esp_timer_handle_t lowBatteryTimer = nullptr;
  ESP_ERROR_CHECK(esp_timer_create(&lowBatteryArgs, &lowBatteryTimer));
  ESP_ERROR_CHECK(esp_timer_start_periodic(lowBatteryTimer, 1000 * 1000));
}


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
      if (currentState == MAIN_USB) {
        currentState = MAIN_NORMAL;
      }
    } else {
      currentState = MAIN_USB;
    }
  });

  Board *board = get_board();

  TaskHandle_t display_task_handle = nullptr;

  dumpDebugTimerInit(board);

  OTA::bootInfo();

  lvgl_init(board);

  initLowBatteryTask();

  vTaskDelay(1000 / portTICK_PERIOD_MS); //Let USB Settle

  if (!board->storageReady()) {
    xTaskNotifyIndexed(display_task_handle, 0, DISPLAY_NO_STORAGE, eSetValueWithOverwrite);
    while (true);
  }

  xTaskCreatePinnedToCore(display_task, "display_task", 5000, board, 2, &display_task_handle, 1);

  MAIN_STATES oldState = MAIN_NONE;
  TaskHandle_t lvglHandle = xTaskGetHandle("LVGL");
  initInputTimer(board);
  while (true) {
    if(currentState == MAIN_NONE){
      currentState = MAIN_NORMAL;
      xTaskNotifyIndexed(display_task_handle, 0, DISPLAY_FILE, eSetValueWithOverwrite);
    }
    if (oldState != currentState) {
      //Handle state transitions
      ESP_LOGI(TAG, "State %d", currentState);
      switch (currentState) {
        case MAIN_NONE:
          break;
        case MAIN_NORMAL:
          if (oldState == MAIN_USB) {
            xTaskNotifyIndexed(lvglHandle, 0, LVGL_STOP, eSetValueWithOverwrite);
            vTaskDelay(100 / portTICK_PERIOD_MS);
            //Check for OTA File
            if (OTA::check()) {
              xTaskNotifyIndexed(display_task_handle, 0, DISPLAY_OTA, eSetValueWithOverwrite);
              vTaskDelay(100 / portTICK_PERIOD_MS);
              OTA::install();
              currentState = MAIN_OTA;
              break;
            }
            else {
              xTaskNotifyIndexed(display_task_handle, 0, DISPLAY_FILE, eSetValueWithOverwrite);
            }
          }
          break;
        case MAIN_USB:
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
    }

    vTaskDelay(200 / portTICK_PERIOD_MS);
  }

}
