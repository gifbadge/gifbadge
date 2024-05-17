#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "input.h"
#include "display.h"
#include "ui/menu.h"

static esp_timer_handle_t inputTimer = nullptr;

extern MAIN_STATES currentState;

static void openMenu() {
  lvgl_menu_open();
}

static void imageCurrent() {
  TaskHandle_t display_task_handle = xTaskGetHandle("display_task");
  xTaskNotifyIndexed(display_task_handle, 0, DISPLAY_FILE, eSetValueWithOverwrite);
}

static void imageNext() {
  TaskHandle_t display_task_handle = xTaskGetHandle("display_task");
  xTaskNotifyIndexed(display_task_handle, 0, DISPLAY_NEXT, eSetValueWithOverwrite);
}

static void imagePrevious() {
  TaskHandle_t display_task_handle = xTaskGetHandle("display_task");
  xTaskNotifyIndexed(display_task_handle, 0, DISPLAY_PREVIOUS, eSetValueWithOverwrite);
}

static void imageSpecial1() {
  TaskHandle_t display_task_handle = xTaskGetHandle("display_task");
  xTaskNotifyIndexed(display_task_handle, 0, DISPLAY_SPECIAL_1, eSetValueWithOverwrite);
}

static void imageSpecial2() {
  TaskHandle_t display_task_handle = xTaskGetHandle("display_task");
  xTaskNotifyIndexed(display_task_handle, 0, DISPLAY_SPECIAL_2, eSetValueWithOverwrite);
}

static const keyCommands keyOptions[KEY_MAX] =
    {keyCommands{imageNext, imageSpecial1}, keyCommands{imagePrevious, imageSpecial2}, keyCommands{openMenu, openMenu}};
EVENT_STATE inputState;
static int lastKey;
static long long lastKeyPress;

#ifdef ESP_PLATFORM
static void inputTimerHandler(void *args) {
  auto board = (Board *) args;
  if (currentState == MAIN_NORMAL) {
    if (!lvgl_menu_state()) {
      EVENT_STATE *key_state = board->getKeys()->read();

      switch (inputState) {
        case STATE_RELEASED:
          for (int b = 0; b < KEY_MAX; b++) {
            if (key_state[b] == STATE_PRESSED) {
              lastKey = b;
              inputState = STATE_PRESSED;
              lastKeyPress = esp_timer_get_time();
            }
          }
          break;
        case STATE_PRESSED:
          if (key_state[lastKey] == STATE_RELEASED) {
            keyOptions[lastKey].press();
            inputState = STATE_RELEASED;
          } else if (esp_timer_get_time() - lastKeyPress > 300 * 1000) {
            if (key_state[lastKey] == STATE_HELD) {
              keyOptions[lastKey].hold();
              inputState = STATE_HELD;
            }
          }
          break;
        case STATE_HELD:
          if (key_state[lastKey] == STATE_RELEASED) {
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
//          LOGI(TAG, "x: %d y: %d", e.first, e.second);
//        }
//      }
    }
  }
}

void initInputTimer(Board *board) {
  const esp_timer_create_args_t inputTimerArgs = {
      .callback = &inputTimerHandler,
      .arg = board,
      .dispatch_method = ESP_TIMER_TASK,
      .name = "input_handler",
      .skip_unhandled_events = true
  };

  ESP_ERROR_CHECK(esp_timer_create(&inputTimerArgs, &inputTimer));
  ESP_ERROR_CHECK(esp_timer_start_periodic(inputTimer, 50 * 1000));
}
#else
void initInputTimer(Board *board) {
}
#endif