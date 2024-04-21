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
#include "config.h"

#include "ota.h"

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
    ESP_LOGI(TAG, "%s", args->board->getBattery()->isCharging() ? "Charging" : "Discharging");

    ESP_LOGI(TAG, "State: %d", static_cast<int>(args->board->getBattery()->status()));


    esp_pm_lock_release(pm_lock);
    vTaskDelay(10000 / portTICK_PERIOD_MS);
  }
}

MAIN_STATES currentState = MAIN_NONE;

/*!
 * Task to check battery status, and update device
 * @param args
 */
static void lowBatteryTask(void *args) {
  std::shared_ptr<Board> board = get_board();
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

static void openMenu(){
  TaskHandle_t display_task_handle = xTaskGetHandle("display_task");
  xTaskNotifyIndexed(display_task_handle, 0, DISPLAY_MENU, eSetValueWithOverwrite);
  vTaskDelay(100 / portTICK_PERIOD_MS);
  lvgl_menu_open();
}

static void imageCurrent(){
  TaskHandle_t display_task_handle = xTaskGetHandle("display_task");
  xTaskNotifyIndexed(display_task_handle, 0, DISPLAY_FILE, eSetValueWithOverwrite);
}

static void imageNext(){
  TaskHandle_t display_task_handle = xTaskGetHandle("display_task");
  xTaskNotifyIndexed(display_task_handle, 0, DISPLAY_NEXT, eSetValueWithOverwrite);
}

static void imagePrevious(){
  TaskHandle_t display_task_handle = xTaskGetHandle("display_task");
  xTaskNotifyIndexed(display_task_handle, 0, DISPLAY_PREVIOUS, eSetValueWithOverwrite);
}

static void imageSpecial1(){
  TaskHandle_t display_task_handle = xTaskGetHandle("display_task");
  xTaskNotifyIndexed(display_task_handle, 0, DISPLAY_SPECIAL_1, eSetValueWithOverwrite);
}

static void imageSpecial2(){
  TaskHandle_t display_task_handle = xTaskGetHandle("display_task");
  xTaskNotifyIndexed(display_task_handle, 0, DISPLAY_SPECIAL_2, eSetValueWithOverwrite);
}

int64_t last_change;
typedef void (*op)();
typedef struct {
  op press;
  op hold;
} keyCommands;
std::map<EVENT_CODE, keyCommands> keyOptions;
EVENT_STATE inputState;
EVENT_CODE lastKey;
long long lastKeyPress;
static void inputTimerHandler(void *args) {
  auto board = (Board *) args;
  if (currentState == MAIN_NORMAL) {
    if (!lvgl_menu_state()) {
      std::map<EVENT_CODE, EVENT_STATE> key_state = board->getKeys()->read();

      switch(inputState){
        case STATE_RELEASED:
          for (auto &button : key_state) {
            if(button.second == STATE_PRESSED){
              lastKey = button.first;
              inputState = STATE_PRESSED;
              lastKeyPress = esp_timer_get_time();
            }
          }
          break;
        case STATE_PRESSED:
          if (key_state[lastKey] == STATE_RELEASED) {
            keyOptions[lastKey].press();
            inputState = STATE_RELEASED;
          } else if(esp_timer_get_time()-lastKeyPress > 300*1000) {
            if (key_state[lastKey] == STATE_HELD) {
              keyOptions[lastKey].hold();
              inputState = STATE_HELD;
            }
          }
          break;
        case STATE_HELD:
          if(key_state[lastKey] == STATE_RELEASED){
            imageCurrent();
            inputState = STATE_RELEASED;
          }
          break;
      }
//TODO: Fix touch
//      if (board->getTouch()) {
//        auto e = board->getTouch()->read();
//        if (e.first > 0 && e.second > 0) {
//          if (((esp_timer_get_time() / 1000) - (last_change / 1000)) > 1000) {
//            last_change = esp_timer_get_time();
//            if (e.second < 50) {
//              openMenu();
//            }
//            if (e.first < 50) {
//              imageNext();
//            }
//            if (e.first > 430) {
//              imagePrevious();
//            }
//          }
//          ESP_LOGI(TAG, "x: %d y: %d", e.first, e.second);
//        }
//      }
    }
  }
}

static void initInputTimer(Board *board) {
  const esp_timer_create_args_t inputTimerArgs = {
      .callback = &inputTimerHandler,
      .arg = board,
      .dispatch_method = ESP_TIMER_TASK,
      .name = "input_handler",
      .skip_unhandled_events = true
  };
  esp_timer_handle_t inputTimer = nullptr;
  ESP_ERROR_CHECK(esp_timer_create(&inputTimerArgs, &inputTimer));
  ESP_ERROR_CHECK(esp_timer_start_periodic(inputTimer, 50 * 1000));
  keyOptions.emplace(KEY_UP, keyCommands{imageNext, imageSpecial1});
  keyOptions.emplace(KEY_DOWN, keyCommands{imagePrevious, imageSpecial2});
  keyOptions.emplace(KEY_ENTER, keyCommands{openMenu, openMenu});
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

  std::shared_ptr<Board> board = get_board();

  auto imageconfig = std::make_shared<ImageConfig>();

  TaskHandle_t display_task_handle = nullptr;

  display_task_args args = {board->getDisplay(), imageconfig, board->getBacklight(),};

  xTaskCreate(display_task, "display_task", 10000, &args, 2, &display_task_handle);

  sharedState configState{imageconfig, board,};

  xTaskCreate(dump_state, "dump_state", 10000, &configState, 2, nullptr);

  OTA::bootInfo();

  lvgl_init(board);

  initLowBatteryTask();

  vTaskDelay(1000 / portTICK_PERIOD_MS); //Let USB Settle

  if (!board->storageReady()) {
    xTaskNotifyIndexed(display_task_handle, 0, DISPLAY_NO_STORAGE, eSetValueWithOverwrite);
    while (true);
  }

  MAIN_STATES oldState = MAIN_NONE;
  last_change = esp_timer_get_time();
  TaskHandle_t lvglHandle = xTaskGetHandle("LVGL");
  initInputTimer(board.get());
  while (true) {
    if(currentState == MAIN_NONE){
      currentState = MAIN_NORMAL;
      imageCurrent();
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
            //Check for OTA File
            if (OTA::check()) {
              OTA::install();
              currentState = MAIN_OTA;
              vTaskDelay(100 / portTICK_PERIOD_MS);
              xTaskNotifyIndexed(display_task_handle, 0, DISPLAY_OTA, eSetValueWithOverwrite);
              break;
            }
          }
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
    }

    vTaskDelay(200 / portTICK_PERIOD_MS);
  }

}
