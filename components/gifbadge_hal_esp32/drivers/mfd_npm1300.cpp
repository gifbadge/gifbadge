#include "drivers/mfd_npm1300.h"

#include "log.h"
#include "npmx_core.h"
#include <esp_timer.h>

static const char *TAG = "mfd_npm1300";

npmx_error_t npmx_write(void *p_context,
                        uint32_t register_address,
                        uint8_t *p_data,
                        size_t num_of_bytes) {
  auto i2c = static_cast<I2C *>(p_context);
//  LOGI(TAG, "npm1300 i2c write %lx bytes %u", register_address, num_of_bytes);
//  for (int i = 0; i < num_of_bytes; i++) {
//    LOGI(TAG, "%x", p_data[i]);
//  }
  i2c->write_reg16(0x6b, static_cast<uint16_t>(register_address), p_data, num_of_bytes);
  return NPMX_SUCCESS;
};

npmx_error_t npmx_read(void *p_context,
                       uint32_t register_address,
                       uint8_t *p_data,
                       size_t num_of_bytes) {
  auto i2c = static_cast<I2C *>(p_context);
//  LOGI(TAG, "npm1300 i2c read %lx bytes %u", register_address, num_of_bytes);
  i2c->read_reg16(0x6b, static_cast<uint16_t>(register_address), p_data, num_of_bytes);
//  for (int i = 0; i < num_of_bytes; i++) {
//    LOGI(TAG, "%x", p_data[i]);
//  }
  return NPMX_SUCCESS;
};

static void npmx_callback(npmx_instance_t *pm, npmx_callback_type_t type, uint8_t mask) {
  LOGI(TAG, "%s:", npmx_callback_to_str(type));
  for (uint8_t i = 0; i < 8; i++) {
    if (BIT(i) & mask) {
      LOGI(TAG, "\t%s", npmx_callback_bit_to_str(type, i));
    }
  }
}

mfd_npm1300::mfd_npm1300(I2C *i2c, gpio_num_t gpio_int)
    : _i2c(i2c), _gpio_int(gpio_int), _npmx_backend(npmx_write, npmx_read, i2c) {

  if (npmx_core_init(&_npmx_instance, &_npmx_backend, npmx_callback, false) != NPMX_SUCCESS) {
    LOGI(TAG, "Unable to init npmx device");
  }
  uint8_t status;
  npmx_vbusin_vbus_status_get(npmx_vbusin_get(&_npmx_instance, 0), &status);
  npmx_vbusin_current_limit_set(npmx_vbusin_get(&_npmx_instance, 0), NPMX_VBUSIN_CURRENT_1000_MA);
  npmx_vbusin_task_trigger(npmx_vbusin_get(&_npmx_instance, 0), NPMX_VBUSIN_TASK_APPLY_CURRENT_LIMIT);
  LOGI(TAG, "npm1300 status %u", status);
}

void mfd_npm1300::poll() {
}

double mfd_npm1300::getVoltage() {
  return _voltage;
}

int mfd_npm1300::getSoc() {
  return _soc;
}

double mfd_npm1300::getRate() {
  return _rate;
}

void mfd_npm1300::removed() {
  present = false;
}

void mfd_npm1300::inserted() {
}

Battery::State mfd_npm1300::status() {
  return Battery::State::ERROR;
}

EVENT_STATE *mfd_npm1300::read() {
  return nullptr;
}

void mfd_npm1300::buck1_set(uint32_t millivolts) {
  LOGI(TAG, "Buck 1 set voltage %lumV", millivolts);
  auto buck = npmx_buck_get(&_npmx_instance, 0);
  npmx_buck_normal_voltage_set(buck, npmx_buck_voltage_convert(millivolts));
  npmx_buck_vout_select_set(buck, NPMX_BUCK_VOUT_SELECT_SOFTWARE);
  npmx_buck_task_trigger(buck, NPMX_BUCK_TASK_ENABLE);
  npmx_buck_status_t status;
  npmx_buck_status_get(buck, &status);
  LOGI(TAG, "Buck status mode %u powered %u pwm %u", status.buck_mode, status.powered, status.pwm_enabled);
}

void mfd_npm1300::buck1_disable() {
  auto buck = npmx_buck_get(&_npmx_instance, 0);
  npmx_buck_task_trigger(buck, NPMX_BUCK_TASK_DISABLE);
}

void mfd_npm1300::loadsw1_enable() {
  auto loadswitch = npmx_ldsw_get(&_npmx_instance, 0);
  npmx_ldsw_active_discharge_enable_set(loadswitch, true);
  npmx_ldsw_task_trigger(loadswitch, NPMX_LDSW_TASK_ENABLE);
}

void mfd_npm1300::loadsw1_disable() {
  auto loadswitch = npmx_ldsw_get(&_npmx_instance, 0);
  npmx_ldsw_task_trigger(loadswitch, NPMX_LDSW_TASK_DISABLE);
}

mfd_npm1300_gpio *mfd_npm1300::gpio_get(uint8_t index) {
  return new mfd_npm1300_gpio(&_npmx_instance, index);
}
mfd_npm1300_shphld *mfd_npm1300::shphld_get() {
  return new mfd_npm1300_shphld(&_npmx_instance);
}

void mfd_npm1300_gpio::config(Gpio::gpio_direction direction, Gpio::gpio_pull_mode pull) {
  npmx_gpio_pull_t pull_mode = NPMX_GPIO_PULL_DOWN;
  switch (pull) {
    case gpio_pull_mode::NONE:
      pull_mode = NPMX_GPIO_PULL_NONE;
      break;
    case gpio_pull_mode::UP:
      pull_mode = NPMX_GPIO_PULL_UP;
      break;
    case gpio_pull_mode::DOWN:
      pull_mode = NPMX_GPIO_PULL_DOWN;
      break;
  }
  _config = {
      .mode = direction == gpio_direction::IN ? NPMX_GPIO_MODE_INPUT : NPMX_GPIO_MODE_OUTPUT_OVERRIDE_0,
      .drive = NPMX_GPIO_DRIVE_6_MA,
      .pull = pull_mode,
      .open_drain = false,
      .debounce = false,
  };
  npmx_gpio_config_set(npmx_gpio_get(_npmx_instance, _index), &_config);
}

bool mfd_npm1300_gpio::read() {
  bool state;
  npmx_gpio_status_check(npmx_gpio_get(_npmx_instance, _index), &state);
  return state;
}

void mfd_npm1300_gpio::write(bool b) {
//  LOGI(TAG, "gpio out %u", b);
  npmx_gpio_mode_set(npmx_gpio_get(_npmx_instance, _index), b?NPMX_GPIO_MODE_OUTPUT_OVERRIDE_1:NPMX_GPIO_MODE_OUTPUT_OVERRIDE_0);
//  bool state;
//  npmx_gpio_status_check(npmx_gpio_get(_npmx_instance, _index), &state);
//  LOGI(TAG, "Desired %u actual %u", b, state);
}

mfd_npm1300_gpio::mfd_npm1300_gpio(npmx_instance_t *npmx, uint8_t index) : _npmx_instance(npmx), _index(index) {

}
void mfd_npm1300_shphld::config(Gpio::gpio_direction direction, Gpio::gpio_pull_mode pull) {

}
bool mfd_npm1300_shphld::read() {
  bool state;
  npmx_ship_gpio_status_check(npmx_ship_get(_npmx_instance, 0), &state);
  return !state;
}
void mfd_npm1300_shphld::write(bool b) {

}
mfd_npm1300_shphld::mfd_npm1300_shphld(npmx_instance_t *npmx) : _npmx_instance(npmx) {

}
