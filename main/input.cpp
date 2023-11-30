#include "freertos/FreeRTOS.h"
#include <freertos/queue.h>
#include <map>
#include <hal/gpio_types.h>
#include <driver/gpio.h>
#include <esp_timer.h>
#include <esp_log.h>
#include "input.h"

static const char *TAG = "INPUT";

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


void input_task(void *arg) {
    auto queue = (QueueHandle_t) arg;
    std::map<EVENT_CODE, button_state> states = {
            {KEY_UP,    {(gpio_num_t) 43,  "up",    false, 0}}, //1
                                                 {KEY_DOWN,  {(gpio_num_t) 44, "down",  false, 0}}, //14
                                                 {KEY_ENTER, {(gpio_num_t) 0,  "enter", false, 0}},};



    for (auto &button: states) {
        gpio_num_t gpio = button.second.pin;
        ESP_LOGI(TAG, "Setting up GPIO %u\n", gpio);
        ESP_ERROR_CHECK(gpio_reset_pin(gpio));
        ESP_ERROR_CHECK(gpio_set_direction(gpio, GPIO_MODE_INPUT));
        ESP_ERROR_CHECK(gpio_set_pull_mode(gpio, GPIO_PULLUP_ONLY));
        ESP_ERROR_CHECK(gpio_pullup_en(gpio));
    }


    while (true) {
        for (auto &button: states) {
            bool state = !gpio_get_level(button.second.pin);
            if (state != button.second.state) {
                button.second.state = state;
                input_event event = {
                        esp_timer_get_time(),
                        button.first,
                        state ? STATE_PRESSED : STATE_RELEASED
                };
                xQueueSendToBack(queue, &event, 100);
            }
        }
        vTaskDelay(KEY_POLL_INTERVAL / portTICK_PERIOD_MS);
    }
}
