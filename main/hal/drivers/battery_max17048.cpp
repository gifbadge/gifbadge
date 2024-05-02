#include "hal/drivers/battery_max17048.h"

#include <utility>
#include <esp_log.h>

static const char *TAG = "battery_max17048";

battery_max17048::battery_max17048(std::shared_ptr<I2C> i2c, gpio_num_t vbus_pin)
    : _i2c(std::move(i2c)), _vbus_pin(vbus_pin) {
  uint8_t data[2];
  _i2c->read_reg(0x36, 0x08, data, 2);
  ESP_LOGI(TAG, "MAX17048 Version %u %u", data[0], data[1]);
  const esp_timer_create_args_t battery_timer_args = {
      .callback = [](void *params) {
        auto bat = (battery_max17048 *) params;
        bat->poll();
      },
      .arg = this,
      .dispatch_method = ESP_TIMER_TASK,
      .name = "battery_max17048",
      .skip_unhandled_events = true
  };
  esp_timer_handle_t battery_handler_handle = nullptr;
  ESP_ERROR_CHECK(esp_timer_create(&battery_timer_args, &battery_handler_handle));
  esp_timer_start_periodic(battery_handler_handle, pollInterval() * 1000);
}

void battery_max17048::poll() {
  uint8_t d[2];
  _i2c->read_reg(0x36, 0x02, d, 2);
  _voltage = ((static_cast<uint16_t>(d[0] << 8) | d[1]) * 78.125) / 1000000;
  _i2c->read_reg(0x36, 0x04, d, 2);
  _soc = static_cast<int>((static_cast<uint16_t>(d[0] << 8) | d[1]) / 256.00);
  _i2c->read_reg(0x36, 0x16, d, 2);
  _rate = (static_cast<int16_t>(d[0] << 8) | d[1]) * 0.208;
}

double battery_max17048::getVoltage() {
  return _voltage;
}

int battery_max17048::getSoc() {
  return _soc;
}

double battery_max17048::getRate() {
  return _rate;
}

void battery_max17048::removed() {
  present = false;
}

void battery_max17048::inserted() {
  // Quickstart. so the MAX17048 restarts it's SOC algorythm.
  //Prevents erroneous readings if battery is swapped while charging
  uint8_t cmd[] = {0x80, 0x00};
  _i2c->write_reg(0x36, 0x06, cmd, 2);
  present = true;
}

Battery::State battery_max17048::status() {
  if(!present){
    return State::NOT_PRESENT;
  }
  if(gpio_get_level(_vbus_pin) && _rate > 1){
    return State::CHARGING;
  }
  if(gpio_get_level(_vbus_pin)){
    return State::CONNECTED_NOT_CHARGING;
  }
  if(!gpio_get_level(_vbus_pin)) {
    return State::DISCHARGING;
  }
  return Battery::State::ERROR;
}
