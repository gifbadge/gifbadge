#include "drivers/battery_max17048.h"

#include "log.h"
#include <esp_timer.h>

static const char *TAG = "battery_max17048";

hal::battery::esp32s3::battery_max17048::battery_max17048(I2C *i2c, gpio_num_t vbus_pin)
    : _i2c(i2c), _vbus_pin(vbus_pin) {
  uint8_t data[2];
  _i2c->read_reg(0x36, 0x08, data, 2);
  LOGI(TAG, "MAX17048 Version %u %u", data[0], data[1]);
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
  esp_timer_start_periodic(battery_handler_handle, 10000 * 1000);
  poll();
}

void hal::battery::esp32s3::battery_max17048::poll() {
  uint8_t d[2];
  _i2c->read_reg(0x36, 0x02, d, 2);
  _voltage = ((static_cast<uint16_t>(d[0] << 8) | d[1]) * 78.125) / 1000000;
  _i2c->read_reg(0x36, 0x04, d, 2);
  _soc = static_cast<int>((static_cast<uint16_t>(d[0] << 8) | d[1]) / 256.00);
  _i2c->read_reg(0x36, 0x16, d, 2);
  _rate = (static_cast<int16_t>(d[0] << 8) | d[1]) * 0.208;
}

double hal::battery::esp32s3::battery_max17048::BatteryVoltage() {
  return _voltage;
}

int hal::battery::esp32s3::battery_max17048::BatterySoc() {
  return _soc;
}

void hal::battery::esp32s3::battery_max17048::BatteryRemoved() {
  present = false;
}

void hal::battery::esp32s3::battery_max17048::BatteryInserted() {
  // Quickstart. so the MAX17048 restarts it's SOC algorythm.
  //Prevents erroneous readings if battery is swapped while charging
  uint8_t cmd[] = {0x80, 0x00};
  _i2c->write_reg(0x36, 0x06, cmd, 2);
  present = true;
}

hal::battery::Battery::State hal::battery::esp32s3::battery_max17048::BatteryStatus() {
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
  return hal::battery::Battery::State::ERROR;
}
