#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <esp_timer.h>
#include <freertos/semphr.h>
#include <esp_log.h>
#include <sys/stat.h>
#include <memory>
#include <utility>
#include <map>
#include "esp_lcd_panel_io.h"
#include <lvgl.h>

#include "ui/menu.h"
#include "hal/battery.h"
#include "ui/main_menu.h"
#include "ui/style.h"
#include "hw_init.h"
#include "display.h"

static const char *TAG = "MENU";

//Static Variables
static lv_disp_t *disp;
static TaskHandle_t lvgl_task;
static flushCbData cbData;
static esp_timer_handle_t lvgl_tick_timer;
static bool menu_state;
static std::shared_ptr<Board> _board;
static SemaphoreHandle_t lvgl_mux = nullptr;





//Exported Variables
extern "C" {
lv_indev_t *lvgl_encoder;
lv_indev_t *lvgl_touch;
}

std::vector<lv_obj_t *> screens;

lv_obj_t * create_screen(){
  lv_obj_t *scr = lv_obj_create(nullptr);
  screens.emplace_back(scr);
  return scr;
}

void destroy_screens(){
  for(auto &scr:screens){
    if(lv_obj_is_valid(scr)) {
      lv_obj_delete(scr);
    }
  }
  screens.clear();
}

static bool IRAM_ATTR flush_ready(esp_lcd_panel_io_handle_t, esp_lcd_panel_io_event_data_t *, void *) {
  lv_display_flush_ready(disp);
  return false;
}

static void flush_cb(lv_disp_t *, const lv_area_t *area, uint8_t *color_map) {
  lv_draw_sw_rgb565_swap(color_map, cbData.display->getResolution().first * cbData.display->getResolution().second);
  cbData.display->write(
      area->x1,
      area->y1,
      area->x2 + 1,
      area->y2 + 1,
      color_map);
}

static void tick([[maybe_unused]] void *arg) {
  lv_tick_inc(LVGL_TICK_PERIOD_MS);
}

bool lvgl_lock(int timeout_ms) {
  // Convert timeout in milliseconds to FreeRTOS ticks
  // If `timeout_ms` is set to -1, the program will block until the condition is met
  const TickType_t timeout_ticks = (timeout_ms == -1) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
  return xSemaphoreTakeRecursive(lvgl_mux, timeout_ticks) == pdTRUE;
}

void lvgl_unlock() {
  xSemaphoreGiveRecursive(lvgl_mux);
}

void lvgl_close() {
  ESP_LOGI(TAG, "Close");
  _board->getDisplay()->onColorTransDone(nullptr, nullptr);

  if (lvgl_lock(-1)) {
    destroy_screens();
    lv_obj_clean(lv_layer_top());
//        lv_disp_remove(disp);
    lvgl_unlock();
  }

  vTaskDelay(100 / portTICK_PERIOD_MS); //Wait some time so the task can finish

  ESP_ERROR_CHECK(esp_timer_stop(lvgl_tick_timer));
  menu_state = false;
  ESP_LOGI(TAG, "Close Done");
  _board->pmRelease();
}

void task(void *) {
  bool running = true;
  ESP_LOGI(TAG, "Starting LVGL task");
  uint32_t task_delay_ms = 0;
  TaskHandle_t handle = xTaskGetHandle("display_task");
  vTaskSuspend(nullptr); //Wait until we are actually needed

  while (running) {
    uint32_t option;
    xTaskNotifyWaitIndexed(0, 0, 0xffffffff, &option, task_delay_ms / portTICK_PERIOD_MS);
    switch (option) {
      case LVGL_STOP:
        lvgl_close();
        vTaskSuspend(nullptr);
        break;
      case LVGL_EXIT:
        running = false;
        break;
      default:
        if (lvgl_lock(-1)) {
          task_delay_ms = lv_timer_handler();
          // Release the mutex
          lvgl_unlock();
        }
        if (task_delay_ms > LVGL_TASK_MAX_DELAY_MS) {
          task_delay_ms = LVGL_TASK_MAX_DELAY_MS;
        } else if (task_delay_ms < LVGL_TASK_MIN_DELAY_MS) {
          task_delay_ms = LVGL_TASK_MIN_DELAY_MS;
        }
        break;
    }
  }
}

void keyboard_read(lv_indev_t *indev, lv_indev_data_t *data) {
//    ESP_LOGI(TAG, "keyboard_read");
  auto g = lv_indev_get_group(indev);
  bool editing = lv_group_get_editing(g);
  Keys *device = static_cast<Keys *>(lv_indev_get_user_data(indev));
  assert(device != nullptr);
  EVENT_STATE *keys = device->read();
  if (keys[KEY_UP] == STATE_PRESSED) {
    data->enc_diff += editing ? +1 : -1;
  } else if (keys[KEY_DOWN] == STATE_PRESSED) {
    data->enc_diff += editing ? -1 : +1;
  } else if (keys[KEY_ENTER] == STATE_PRESSED || keys[KEY_ENTER] == STATE_HELD) {
    data->state = LV_INDEV_STATE_PRESSED;
  } else {
    data->state = LV_INDEV_STATE_RELEASED;
  }

}

bool lvgl_menu_state() {
  return menu_state;
}

void touch_read(lv_indev_t *drv, lv_indev_data_t *data) {

  auto touch = static_cast<Touch *>(lv_indev_get_driver_data(drv));
  auto i = touch->read();
  if (i.first > 0 && i.second > 0) {
    data->point.x = (int32_t) i.first;
    data->point.y = (int32_t) i.second;
    data->state = LV_INDEV_STATE_PRESSED;
  } else {
    data->state = LV_INDEV_STATE_RELEASED;
  }
}

void lvgl_init(std::shared_ptr<Board> board) {
  _board = std::move(board);

  menu_state = false;

  lv_init();

  lvgl_mux = xSemaphoreCreateRecursiveMutex();

  const esp_timer_create_args_t lvgl_tick_timer_args = {
      .callback = &tick,
      .arg = nullptr,
      .dispatch_method = ESP_TIMER_TASK,
      .name = "lvgl_tick",
      .skip_unhandled_events = true
  };
  lvgl_tick_timer = nullptr;
  ESP_ERROR_CHECK(esp_timer_create(&lvgl_tick_timer_args, &lvgl_tick_timer));

  xTaskCreate(task, "LVGL", 10000, nullptr, LVGL_TASK_PRIORITY, &lvgl_task);

  disp = lv_display_create(_board->getDisplay()->getResolution().first, _board->getDisplay()->getResolution().second);
  lv_display_set_flush_cb(disp, flush_cb);
  size_t buffer_size = _board->getDisplay()->getResolution().first * _board->getDisplay()->getResolution().second * 2;
  ESP_LOGI(TAG, "Display Buffer Size %u", buffer_size);
  if (_board->getDisplay()->directRender()) {
    void *buffer = heap_caps_malloc(buffer_size, MALLOC_CAP_SPIRAM);
    lv_display_set_buffers(disp, buffer, nullptr, buffer_size,
                           LV_DISPLAY_RENDER_MODE_FULL);
  } else {
    lv_display_set_buffers(disp, _board->getDisplay()->getBuffer(), nullptr, buffer_size,
                           LV_DISPLAY_RENDER_MODE_FULL);
  }

  style_init();
  lvgl_encoder = lv_indev_create();
  lv_indev_set_type(lvgl_encoder, LV_INDEV_TYPE_ENCODER);
  lv_indev_set_user_data(lvgl_encoder, _board->getKeys().get());
  lv_indev_set_read_cb(lvgl_encoder, keyboard_read);
  lv_timer_set_period(lv_indev_get_read_timer(lvgl_encoder), 50);
//
//
  if (_board->getTouch()) {
    lvgl_touch = lv_indev_create();
    lv_indev_set_type(lvgl_touch, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(lvgl_touch, touch_read);
    lv_indev_set_driver_data(lvgl_touch, _board->getTouch().get());
    lv_timer_set_period(lv_indev_get_read_timer(lvgl_touch), 150);
  }

}

void battery_update(lv_obj_t *widget) {
  auto battery = static_cast<Battery *>(lv_obj_get_user_data(widget));
  if (battery->status() == Battery::State::ERROR) {
    lv_obj_add_state(widget, LV_STATE_CHECKED);
    lv_label_set_text(widget, ICON_BATTERY_ALERT);
  } else if (battery->status() == Battery::State::NOT_PRESENT) {
    lv_obj_clear_state(widget, LV_STATE_CHECKED);
    lv_label_set_text(widget, ICON_BATTERY_REMOVED);
  } else if (battery->status() == Battery::State::CHARGING) {
    lv_obj_clear_state(widget, LV_STATE_CHECKED);
    if (battery->getSoc() > 90) {
      lv_label_set_text(widget, ICON_BATTERY_100);
    } else if (battery->getSoc() > 80) {
      lv_label_set_text(widget, ICON_BATTERY_CHARGING_80);
    } else if (battery->getSoc() > 50) {
      lv_label_set_text(widget, ICON_BATTERY_CHARGING_50);
    } else if (battery->getSoc() > 30) {
      lv_label_set_text(widget, ICON_BATTERY_CHARGING_30);
    } else {
      lv_label_set_text(widget, ICON_BATTERY_CHARGING_0);
    }
  } else {
    if (battery->getSoc() > 90) {
      lv_obj_clear_state(widget, LV_STATE_CHECKED);
      lv_label_set_text(widget, ICON_BATTERY_100);
    } else if (battery->getSoc() > 80) {
      lv_obj_clear_state(widget, LV_STATE_CHECKED);
      lv_label_set_text(widget, ICON_BATTERY_80);
    } else if (battery->getSoc() > 50) {
      lv_obj_clear_state(widget, LV_STATE_CHECKED);
      lv_label_set_text(widget, ICON_BATTERY_50);

    } else if (battery->getSoc() > 30) {
      lv_obj_clear_state(widget, LV_STATE_CHECKED);
      lv_label_set_text(widget, ICON_BATTERY_30);
    } else {
      lv_obj_add_state(widget, LV_STATE_CHECKED);
      lv_label_set_text(widget, ICON_BATTERY_0);
    }
  }
}



static void battery_widget(lv_obj_t *scr) {
  lv_obj_t *battery_bar = lv_obj_create(scr);
  lv_obj_set_size(battery_bar, lv_pct(100), LV_SIZE_CONTENT);
  lv_obj_set_style_bg_color(battery_bar, lv_obj_get_style_bg_color(lv_scr_act(), LV_PART_MAIN), LV_PART_MAIN);
  lv_obj_set_style_border_side(battery_bar, LV_BORDER_SIDE_NONE, LV_PART_MAIN);
  lv_obj_set_style_pad_all(battery_bar, 0, LV_PART_MAIN);

  lv_obj_t *battery = lv_label_create(battery_bar);
  lv_obj_set_width(battery, LV_PCT(100));
  lv_obj_add_style(battery, &battery_style_normal, 0);
  lv_obj_add_style(battery, &battery_style_empty, LV_STATE_CHECKED);
  lv_obj_add_style(battery, &icon_style, 0);
  lv_obj_set_pos(battery, 0, 10);
  lv_label_set_text(battery, ICON_BATTERY_0);
  lv_obj_add_state(battery, LV_STATE_CHECKED);

  lv_obj_set_user_data(battery, _board->getBattery().get());


  //TODO: See why this causes a freeze in LVGL
  lv_obj_add_event_cb(battery, [](lv_event_t *e) {
    battery_update(static_cast<lv_obj_t *>(lv_event_get_target(e)));
  }, LV_EVENT_REFRESH, nullptr);
  battery_update(battery);

  lv_timer_t *timer = lv_timer_create([](lv_timer_t *timer) {
    auto *obj = (lv_obj_t *) timer->user_data;
    lv_obj_send_event(obj, LV_EVENT_REFRESH, nullptr);
  }, 30000, battery);
  lv_obj_add_event_cb(battery, [](lv_event_t *e) {
    auto *timer = (lv_timer_t *) e->user_data;
    lv_timer_del(timer);
  }, LV_EVENT_DELETE, timer);

}

void lvgl_wake_up() {
  if(!menu_state) {
    ESP_LOGI(TAG, "Wakeup");
    _board->pmLock();
    menu_state = true;

    cbData.display = _board->getDisplay();

    cbData.callbackEnabled = _board->getDisplay()->onColorTransDone(flush_ready, &disp);

    if (_board->getDisplay()->directRender()) {
      _board->getDisplay()->write(0, 0, _board->getDisplay()->getResolution().first,
                                  _board->getDisplay()->getResolution().second, _board->getDisplay()->getBuffer2());
    }
    lv_display_flush_ready(disp); //Always start ready
    vTaskResume(lvgl_task);

    ESP_ERROR_CHECK(esp_timer_start_periodic(lvgl_tick_timer, LVGL_TICK_PERIOD_MS * 1000));
    if (lvgl_lock(-1)) {
      lv_group_t *g = lv_group_create();
      lv_group_set_default(g);
      lv_indev_set_group(lvgl_encoder, g);
      lv_obj_t *scr = create_screen();
      lv_screen_load(scr);
      battery_widget(lv_layer_top());
      lvgl_unlock();
    }
    ESP_LOGI(TAG, "Wakeup Done");
  }
}

void lvgl_menu_open() {
  lvgl_wake_up();
  main_menu();
}



