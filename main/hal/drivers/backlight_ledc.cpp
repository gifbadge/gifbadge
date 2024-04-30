//
// Created by gifbadge on 27/12/23.
//

#include <driver/ledc.h>
#include <esp_log.h>
#include "hal/drivers/backlight_ledc.h"

static const char *TAG = "backlight_gpio.cpp";

backlight_ledc::backlight_ledc(gpio_num_t gpio, int level) : lastLevel(level) {
  ESP_LOGI(TAG, "Turn on LCD backlight");

  // Prepare and then apply the LEDC PWM timer configuration
  ledc_timer_config_t ledc_timer = {
      .speed_mode       = LEDC_LOW_SPEED_MODE,
      .duty_resolution  = LEDC_TIMER_8_BIT,
      .timer_num        = LEDC_TIMER_0,
      .freq_hz          = 4000,  // Set output frequency at 4 kHz
      .clk_cfg          = LEDC_USE_RC_FAST_CLK
  };
  ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

  // Prepare and then apply the LEDC PWM channel configuration
  ledc_channel_config_t ledc_channel = {
      .gpio_num       = (gpio_num_t) gpio,
      .speed_mode     = LEDC_LOW_SPEED_MODE,
      .channel        = LEDC_CHANNEL_0,
      .intr_type      = LEDC_INTR_DISABLE,
      .timer_sel      = LEDC_TIMER_0,
      .duty           = 0, // Set duty to 0%
      .hpoint         = 0,
      .flags          = {.output_invert = 0}
  };
  ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
  ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, static_cast<uint32_t>(256 / (level / 100.00)) - 1);
  ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
  ESP_ERROR_CHECK(gpio_sleep_sel_dis((gpio_num_t) gpio));
}

void backlight_ledc::state(bool state) {
  if (state) {
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, static_cast<uint32_t>(256 / (lastLevel / 100.00)) - 1);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
  } else {
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
  }
}

void backlight_ledc::setLevel(int level) {
  lastLevel = level;
  uint32_t duty = (level*256)/100;
  ESP_LOGI(TAG, "backlight level: %lu\n", duty);
  ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty);
  ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
}

int backlight_ledc::getLevel() {
  return lastLevel;
}
