#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_pm.h"

#include "esp_err.h"
#include "esp_log.h"

#include "log.h"

#include <cstring>

#include "ui/menu.h"
#include "hal/esp32/hal_usb.h"
#include "display.h"

#include "ota.h"

#include "hw_init.h"
#include "ui/usb_connected.h"
#include "input.h"

static const char *TAG = "MAIN";

void dumpDebugFunc(void *arg) {
  auto *args = (Board *) arg;
//  esp_pm_lock_handle_t pm_lock;
//  esp_pm_lock_create(ESP_PM_NO_LIGHT_SLEEP, 0, "output_lock", &pm_lock);

//    esp_pm_lock_acquire(pm_lock);
//        vTaskDelay(1000/portTICK_PERIOD_MS);
  if (true) {
    esp_pm_dump_locks(stdout);
//      char out[1000];
//      vTaskGetRunTimeStats(out);
//      printf("%s", out);
  }
  LOGI(TAG, "SOC: %i", args->getBattery()->getSoc());
  LOGI(TAG, "Voltage: %f", args->getBattery()->getVoltage());
//    LOGI(TAG, "Rate: %f", args->getBattery()->getRate());
//    LOGI(TAG, "State: %d", static_cast<int>(args->getBattery()->status()));
  heap_caps_print_heap_info(MALLOC_CAP_INTERNAL);

  TaskStatus_t tasks[20];
  unsigned int count = uxTaskGetSystemState(tasks, 20, nullptr);
  for (unsigned int i = 0; i < count; i++) {
    LOGI(TAG, "%s Highwater: %lu", tasks[i].pcTaskName, tasks[i].usStackHighWaterMark);
  }

  heap_caps_print_heap_info(MALLOC_CAP_SPIRAM);

//    char task_name[] = "timer_task";
//  TaskHandle_t handle = xTaskGetHandle(task_name);
//  LOGI(TAG, "%s Highwater: %d", task_name, uxTaskGetStackHighWaterMark(handle));


//  esp_pm_lock_release(pm_lock);
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
  auto *board = static_cast<Board *>(args);
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

static void initLowBatteryTask(Board *board) {
  const esp_timer_create_args_t lowBatteryArgs = {
      .callback = &lowBatteryTask,
      .arg = board,
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
//  heap_caps_print_heap_info(MALLOC_CAP_INTERNAL);
  Board *board = get_board();
//  heap_caps_print_heap_info(MALLOC_CAP_INTERNAL);


  storage_callback([](bool state) {
    LOGI(TAG, "state %u", state);
    if (state) {
      if (currentState == MAIN_USB) {
        currentState = MAIN_NORMAL;
      }
    } else {
      currentState = MAIN_USB;
    }
  });
//  heap_caps_print_heap_info(MALLOC_CAP_INTERNAL);

  TaskHandle_t display_task_handle = nullptr;

  dumpDebugTimerInit(board);

  OTA::bootInfo();

  lvgl_init(board);

  initLowBatteryTask(board);

  vTaskDelay(1000 / portTICK_PERIOD_MS); //Let USB Settle

  xTaskCreatePinnedToCore(display_task, "display_task", 5000, board, 2, &display_task_handle, 1);


  if (!board->storageReady()) {
    xTaskNotifyIndexed(display_task_handle, 0, DISPLAY_NO_STORAGE, eSetValueWithOverwrite);
    while (true);
  }


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
      LOGI(TAG, "State %d", currentState);
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
