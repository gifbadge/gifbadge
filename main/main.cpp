#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "log.h"

#include <esp_pm.h>

#include "ui/menu.h"
#include "hal/esp32/hal_usb.h"
#include "display.h"

#include "ota.h"

#include "hw_init.h"
#include "ui/usb_connected.h"
#include "input.h"

static const char *TAG = "MAIN";

void dumpDebugFunc(TimerHandle_t) {
  auto *args = get_board();
  args->pmLock();

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
  args->pmRelease();

}

static void dumpDebugTimerInit() {
  xTimerStart(xTimerCreate("dumpDebugTimer", 10000/portTICK_PERIOD_MS, pdTRUE, nullptr, dumpDebugFunc), 0);
}

MAIN_STATES currentState = MAIN_NONE;

/*!
 * Task to check battery status, and update device
 * @param args
 */
static void lowBatteryTask(TimerHandle_t) {
  auto *board = get_board();
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
  xTimerStart(xTimerCreate("low_battery_handler", 1000/portTICK_PERIOD_MS, pdTRUE, nullptr, lowBatteryTask),0);
}


extern "C" void app_main(void) {
  Board *board = get_board();

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

  TaskHandle_t display_task_handle = nullptr;

  dumpDebugTimerInit();

  OTA::bootInfo();

  lvgl_init(board);

  initLowBatteryTask();

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
