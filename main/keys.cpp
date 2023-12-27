#include "freertos/FreeRTOS.h"
#include <freertos/queue.h>
#include <map>
#include <hal/gpio_types.h>
#include <esp_timer.h>
#include <esp_log.h>
#include <mutex>
#include "keys.h"

static const char *TAG = "INPUT";

static std::mutex input_lock;

void get_event(input_event i) {
    switch (i.code) {
        case KEY_UP:
            ESP_LOGI(TAG, "%lli KEY_UP %s\n", i.timestamp / 1000, i.value ? "PRESSED" : "RELEASED");
            break;
        case KEY_DOWN:
            ESP_LOGI(TAG, "%lli KEY_DOWN %s\n", i.timestamp / 1000, i.value ? "PRESSED" : "RELEASED");
            break;
        case KEY_ENTER:
            ESP_LOGI(TAG, "%lli KEY_ENTER %s\n", i.timestamp / 1000, i.value ? "PRESSED" : "RELEASED");
            break;
        default:
            ESP_LOGI(TAG, "%i Unknown Code\n", i.code);
            break;
    }
}

std::map<EVENT_CODE, EVENT_STATE> current_state =
        {{KEY_UP,    STATE_RELEASED},
         {KEY_DOWN,  STATE_RELEASED},
         {KEY_ENTER, STATE_RELEASED}
        };

static void input_timer_handler(void *arg) {
    auto *args = (keyArgs *) arg;

    const std::lock_guard<std::mutex> lock(input_lock);

    for (auto &button: args->keys->read()) {
        if (current_state[button.first] != button.second) {
            current_state[button.first] = button.second;
            input_event event = {
                    esp_timer_get_time(),
                    button.first,
                    button.second
            };
            xQueueSendToBackFromISR(args->queue, &event, 0);
        }
    }
}

void input_init(const std::shared_ptr<Keys> &keys, QueueHandle_t key_queue) {
    ESP_LOGI(TAG, "input_init");
    const std::lock_guard<std::mutex> lock(input_lock);
    auto *_keyArgs = static_cast<keyArgs *>(malloc(sizeof(keyArgs)));
    _keyArgs->keys = keys;
    _keyArgs->queue = key_queue;

    const esp_timer_create_args_t input_timer_args = {.callback = &input_timer_handler, .arg = _keyArgs, .name = "key_handler"};
    esp_timer_handle_t key_handler_handle = nullptr;
    ESP_ERROR_CHECK(esp_timer_create(&input_timer_args, &key_handler_handle));
    esp_timer_start_periodic(key_handler_handle, keys->pollInterval() * 1000);
}

std::map<EVENT_CODE, EVENT_STATE> input_read() {
    const std::lock_guard<std::mutex> lock(input_lock);
    return current_state;
}