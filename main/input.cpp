#include "FreeRTOS.h"
#include "task.h"
#include "input.h"
#include "display.h"
#include "ui/menu.h"
#include "hw_init.h"

extern MAIN_STATES currentState;

static void openMenu() {
  if (!lvgl_menu_state() && currentState == MAIN_NORMAL) {
    lvgl_menu_open();
  }
}

static void powerOff() {
  get_board()->PowerOff();
}

static void imageCurrent() {
  if (!lvgl_menu_state() && currentState == MAIN_NORMAL) {
    TaskHandle_t display_task_handle = xTaskGetHandle("display_task");
    xTaskNotifyIndexed(display_task_handle, 0, DISPLAY_FILE, eSetValueWithOverwrite);
  }
}

static void imageNext() {
  if (!lvgl_menu_state() && currentState == MAIN_NORMAL) {
    TaskHandle_t display_task_handle = xTaskGetHandle("display_task");
    xTaskNotifyIndexed(display_task_handle, 0, DISPLAY_NEXT, eSetValueWithOverwrite);
  }
}

static void imagePrevious() {
  if (!lvgl_menu_state() && currentState == MAIN_NORMAL) {
    TaskHandle_t display_task_handle = xTaskGetHandle("display_task");
    xTaskNotifyIndexed(display_task_handle, 0, DISPLAY_PREVIOUS, eSetValueWithOverwrite);
  }
}

static void imageSpecial1() {
  if (!lvgl_menu_state() && currentState == MAIN_NORMAL) {
    TaskHandle_t display_task_handle = xTaskGetHandle("display_task");
    xTaskNotifyIndexed(display_task_handle, 0, DISPLAY_SPECIAL_1, eSetValueWithOverwrite);
  }
}

static void imageSpecial2() {
  if (!lvgl_menu_state() && currentState == MAIN_NORMAL) {
    TaskHandle_t display_task_handle = xTaskGetHandle("display_task");
    xTaskNotifyIndexed(display_task_handle, 0, DISPLAY_SPECIAL_2, eSetValueWithOverwrite);
  }
}

static const keyCommands keyOptions[KEY_MAX] = {
    keyCommands{imageNext, imageSpecial1, 300*1000},
    keyCommands{imagePrevious, imageSpecial2, 300*1000},
    keyCommands{openMenu, powerOff, 5000*1000}
};

EVENT_STATE inputState;
static int lastKey;
static long long lastKeyPress;

#ifdef ESP_PLATFORM
#include <esp_timer.h>
static esp_timer_handle_t inputTimer = nullptr;

static void inputTimerHandler(void *args) {
  auto board = (Boards::Board *) args;
  EVENT_STATE *key_state = board->GetKeys()->read();

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
      } else if (esp_timer_get_time() - lastKeyPress > keyOptions[lastKey].delay) {
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

void initInputTimer(Boards::Board *board) {
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
#include "portable_time.h"
#include "timers.h"

static void inputTimerHandler(TimerHandle_t) {
  auto board = get_board();
  if (currentState == MAIN_NORMAL) {
    if (!lvgl_menu_state()) {
      EVENT_STATE *key_state = board->getKeys()->read();

      switch (inputState) {
        case STATE_RELEASED:
          for (int b = 0; b < KEY_MAX; b++) {
            if (key_state[b] == STATE_PRESSED) {
              lastKey = b;
              inputState = STATE_PRESSED;
              lastKeyPress = millis();
            }
          }
          break;
        case STATE_PRESSED:
          if (key_state[lastKey] == STATE_RELEASED) {
            keyOptions[lastKey].press();
            inputState = STATE_RELEASED;
          } else if (millis() - lastKeyPress > 300 * 1000) {
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
  xTimerStart(xTimerCreate("input", 5/portTICK_PERIOD_MS, pdTRUE, nullptr, inputTimerHandler),0);
}
#endif