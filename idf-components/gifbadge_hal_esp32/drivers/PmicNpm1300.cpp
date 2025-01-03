#include "drivers/PmicNpm1300.h"

#include "log.h"
#include "npmx_core.h"
#include <esp_timer.h>
#include <soc/gpio_sig_map.h>
#include <cmath>

static const char *TAG = "PmicNpm1300";

static void irq(void *arg) {
  auto pmic = static_cast<hal::pmic::esp32s3::PmicNpm1300 *>(arg);
  pmic->HandleInterrupt();
}

static void npmx_timer(void *arg) {
  auto pmic = static_cast<hal::pmic::esp32s3::PmicNpm1300 *>(arg);
  pmic->Loop();
}

npmx_error_t npmx_write(void *p_context, uint32_t register_address, uint8_t *p_data, size_t num_of_bytes) {
  auto i2c = static_cast<I2C *>(p_context);
//  LOGI(TAG, "npm1300 i2c write %lx bytes %u", register_address, num_of_bytes);
//  for (int i = 0; i < num_of_bytes; i++) {
//    LOGI(TAG, "%x", p_data[i]);
//  }
  i2c->write_reg16(0x6b, static_cast<uint16_t>(register_address), p_data, num_of_bytes);
  return NPMX_SUCCESS;
}

npmx_error_t npmx_read(void *p_context, uint32_t register_address, uint8_t *p_data, size_t num_of_bytes) {
  auto i2c = static_cast<I2C *>(p_context);
//  LOGI(TAG, "npm1300 i2c read %lx bytes %u", register_address, num_of_bytes);
  i2c->read_reg16(0x6b, static_cast<uint16_t>(register_address), p_data, num_of_bytes);
//  for (int i = 0; i < num_of_bytes; i++) {
//    LOGI(TAG, "%x", p_data[i]);
//  }
  return NPMX_SUCCESS;
}

void hal::pmic::esp32s3::PmicNpm1300::VbusVoltage(npmx_instance_t *pm, npmx_callback_type_t, uint8_t mask) {
  if (mask & (NPMX_EVENT_GROUP_VBUSIN_DETECTED_MASK | NPMX_EVENT_GROUP_VBUSIN_REMOVED_MASK)) {
    auto *pmic = static_cast<hal::pmic::esp32s3::PmicNpm1300 *>(npmx_core_context_get(pm));
    if (mask & NPMX_EVENT_GROUP_VBUSIN_DETECTED_MASK) {
      LOGI(TAG, "VBUS CONNECTED");
      if(pmic->_vbus_callback){
        pmic->_vbus_callback(true);
      }
      if(pmic->_power_led != nullptr){
        pmic->_power_led->GpioWrite(true);
      }
    } else {
      LOGI(TAG, "VBUS DISCONNECTED");
      if(pmic->_vbus_callback){
        pmic->_vbus_callback(false);
      }
      if(pmic->_power_led != nullptr) {
        pmic->_power_led->GpioWrite(false);
      }
    }
  }
}

static void cc_set_current(npmx_instance_t *pm) {
  npmx_vbusin_cc_t cc1;
  npmx_vbusin_cc_t cc2;
  npmx_vbusin_cc_t selected;
  npmx_vbusin_cc_status_get(npmx_vbusin_get(pm, 0), &cc1, &cc2);
  selected = cc1 >= cc2 ? cc1 : cc2;
  LOGI(TAG, "USB power detected %s", npmx_vbusin_cc_status_map_to_string(selected));
  npmx_vbusin_cc_status_get(npmx_vbusin_get(pm, 0), &cc1, &cc2);
  if (selected >= 2) {
    npmx_vbusin_current_limit_set(npmx_vbusin_get(pm, 0), NPMX_VBUSIN_CURRENT_1000_MA);
  } else {
    npmx_vbusin_current_limit_set(npmx_vbusin_get(pm, 0), NPMX_VBUSIN_CURRENT_500_MA);
  }
  npmx_vbusin_task_trigger(npmx_vbusin_get(pm, 0), NPMX_VBUSIN_TASK_APPLY_CURRENT_LIMIT);
}

static void vbus_thermal(npmx_instance_t *pm, npmx_callback_type_t, uint8_t mask) {
  if (mask & (NPMX_EVENT_GROUP_USB_CC1_MASK | NPMX_EVENT_GROUP_USB_CC2_MASK)) {
    cc_set_current(pm);
  }
}

static void npmx_callback(npmx_instance_t *pm, npmx_callback_type_t type, uint8_t mask) {
  LOGI(TAG, "%s:", npmx_callback_to_str(type));
  for (uint8_t i = 0; i < 8; i++) {
    if (BIT(i) & mask) {
      LOGI(TAG, "\t%s", npmx_callback_bit_to_str(type, i));
    }
  }
  uint8_t status;
  npmx_vbusin_vbus_status_get(npmx_vbusin_get(pm, 0), &status);
  LOGI(TAG, "npm1300 status %u", status);
}

static Boards::WakeupSource get_wakeup(npmx_instance_t *pm){
  Boards::WakeupSource wakeup = Boards::WakeupSource::NONE;
  uint8_t flags;
  npmx_backend_register_read(pm->p_backend, NPMX_REG_TO_ADDR(NPM_MAIN->EVENTSVBUSIN0SET), &flags, 1);
  if(flags & NPMX_EVENT_GROUP_VBUSIN_DETECTED_MASK){
    return Boards::WakeupSource::VBUS;
  }

  npmx_backend_register_read(pm->p_backend, NPMX_REG_TO_ADDR(NPM_MAIN->EVENTSSHPHLDSET), &flags, 1);
  if(flags & NPMX_EVENT_GROUP_SHIPHOLD_PRESSED_MASK){
    return Boards::WakeupSource::KEY;
  }
  return wakeup;
}

hal::pmic::esp32s3::PmicNpm1300::PmicNpm1300(I2C *i2c, gpio_num_t gpio_int)
    : _i2c(i2c), _gpio_int(gpio_int), _npmx_backend(npmx_write, npmx_read, i2c) {

  if (npmx_core_init(&_npmx_instance, &_npmx_backend, npmx_callback, false) != NPMX_SUCCESS) {
    LOGI(TAG, "Unable to init npmx device");
  }

  npmx_core_context_set(&_npmx_instance, this);

  npmx_core_event_interrupt_disable(&_npmx_instance, NPMX_EVENT_GROUP_GPIO, NPMX_EVENT_GROUP_GPIO0_DETECTED_MASK|NPMX_EVENT_GROUP_GPIO1_DETECTED_MASK|NPMX_EVENT_GROUP_GPIO2_DETECTED_MASK|NPMX_EVENT_GROUP_GPIO3_DETECTED_MASK|NPMX_EVENT_GROUP_GPIO4_DETECTED_MASK);

}

void hal::pmic::esp32s3::PmicNpm1300::poll() {
}

double hal::pmic::esp32s3::PmicNpm1300::BatteryVoltage() {
  return _vbat/1000.00;
}

static inline uint8_t sigmoidal(uint16_t voltage, uint16_t minVoltage, uint16_t maxVoltage) {
  auto result =
      static_cast<uint8_t>(105 - (105 / (1 + pow(1.724 * (voltage - minVoltage) / (maxVoltage - minVoltage), 5.5))));
  return result >= 100 ? 100 : result;
}

static inline uint8_t asigmoidal(uint16_t voltage, uint16_t minVoltage = 2800, uint16_t maxVoltage = 4200) {
  if(voltage < minVoltage){
    return 0;
  }
  if(voltage > maxVoltage){
    return 100;
  }
//  uint8_t result = 101 - (101 / pow(1 + pow(1.33 * (voltage - minVoltage)/(maxVoltage - minVoltage) ,4.5), 3));
  uint8_t result = 100 - (100 / pow(1 + pow(1.6 * (voltage - minVoltage)/(maxVoltage - minVoltage) ,5.5), 1.2));
  return result >= 100 ? 100 : result;
}

int hal::pmic::esp32s3::PmicNpm1300::BatterySoc() {
  uint8_t status;
  npmx_charger_status_get(npmx_charger_get(&_npmx_instance, 0), &status);
  if(status & NPMX_CHARGER_STATUS_COMPLETED_MASK){
    return 100;
  }
  return asigmoidal(static_cast<uint16_t>(_vbat));
}

void hal::pmic::esp32s3::PmicNpm1300::BatteryRemoved() {
  _present = false;
}

void hal::pmic::esp32s3::PmicNpm1300::BatteryInserted() {
}

hal::battery::Battery::State hal::pmic::esp32s3::PmicNpm1300::BatteryStatus() {
  uint8_t status;
  npmx_charger_status_get(npmx_charger_get(&_npmx_instance, 0), &status);
  if(!(status & NPMX_CHARGER_STATUS_BATTERY_DETECTED_MASK)){
    return State::NOT_PRESENT;
  }
  if(_charge_error != Charger::ChargeError::NONE){
    return State::ERROR;
  }
  if(status & (NPMX_CHARGER_STATUS_TRICKLE_CHARGE_MASK | NPMX_CHARGER_STATUS_CONSTANT_CURRENT_MASK | NPMX_CHARGER_STATUS_CONSTANT_VOLTAGE_MASK)){
    return State::CHARGING;
  }
  if(!VbusConnected()){
    return State::DISCHARGING;
  }
  return State::OK;
}

void hal::pmic::esp32s3::PmicNpm1300::Buck1Set(uint32_t millivolts) {
  LOGI(TAG, "Buck 1 set voltage %lumV", millivolts);
  auto buck = npmx_buck_get(&_npmx_instance, 0);
  npmx_buck_normal_voltage_set(buck, npmx_buck_voltage_convert(millivolts));
  npmx_buck_vout_select_set(buck, NPMX_BUCK_VOUT_SELECT_SOFTWARE);
  npmx_buck_task_trigger(buck, NPMX_BUCK_TASK_ENABLE);
  npmx_buck_status_t status;
  npmx_buck_status_get(buck, &status);
  LOGI(TAG, "Buck status mode %u powered %u pwm %u", status.buck_mode, status.powered, status.pwm_enabled);
}

void hal::pmic::esp32s3::PmicNpm1300::Buck1Disable() {
  auto buck = npmx_buck_get(&_npmx_instance, 0);
  npmx_buck_task_trigger(buck, NPMX_BUCK_TASK_DISABLE);
}

void hal::pmic::esp32s3::PmicNpm1300::LoadSw1Enable() {
  auto loadswitch = npmx_ldsw_get(&_npmx_instance, 0);
  npmx_ldsw_active_discharge_enable_set(loadswitch, true);
  npmx_ldsw_task_trigger(loadswitch, NPMX_LDSW_TASK_ENABLE);
}

void hal::pmic::esp32s3::PmicNpm1300::LoadSw1Disable() {
  auto loadswitch = npmx_ldsw_get(&_npmx_instance, 0);
  npmx_ldsw_task_trigger(loadswitch, NPMX_LDSW_TASK_DISABLE);
}

hal::pmic::esp32s3::PmicNpm1300Gpio *hal::pmic::esp32s3::PmicNpm1300::GpioGet(uint8_t index) {
  return new PmicNpm1300Gpio(&_npmx_instance, index);
}
hal::pmic::esp32s3::PmicNpm1300ShpHld *hal::pmic::esp32s3::PmicNpm1300::ShphldGet() {
  return new PmicNpm1300ShpHld(&_npmx_instance);
}
hal::pmic::esp32s3::PmicNpm1300Led *hal::pmic::esp32s3::PmicNpm1300::LedGet(uint8_t index) {
  return new PmicNpm1300Led(&_npmx_instance, index);
}
void hal::pmic::esp32s3::PmicNpm1300::ChargeEnable() {
  npmx_charger_task_trigger(npmx_charger_get(&_npmx_instance, 0),NPMX_CHARGER_TASK_CLEAR_ERROR);
  npmx_charger_task_trigger(npmx_charger_get(&_npmx_instance, 0),NPMX_CHARGER_TASK_CLEAR_TIMERS);
  npmx_charger_module_enable_set(npmx_charger_get(&_npmx_instance, 0),
                                 NPMX_CHARGER_MODULE_CHARGER_MASK);
}
void hal::pmic::esp32s3::PmicNpm1300::ChargeDisable() {
  npmx_charger_module_disable_set(npmx_charger_get(&_npmx_instance, 0),
                                  NPMX_CHARGER_MODULE_CHARGER_MASK);
}
void hal::pmic::esp32s3::PmicNpm1300::ChargeCurrentSet(uint16_t iset) {
  uint16_t set = ((iset + 1) / 2) * 2;
  npmx_charger_charging_current_set(npmx_charger_get(&_npmx_instance, 0), set);
}
uint16_t hal::pmic::esp32s3::PmicNpm1300::ChargeCurrentGet() {
  uint16_t iset;
  npmx_charger_charging_current_get(npmx_charger_get(&_npmx_instance, 0), &iset);
  return iset;
}
void hal::pmic::esp32s3::PmicNpm1300::DischargeCurrentSet(uint16_t iset) {
  uint16_t set = ((iset + 1) / 2) * 2;
  npmx_charger_discharging_current_set(npmx_charger_get(&_npmx_instance, 0), set);
}
uint16_t hal::pmic::esp32s3::PmicNpm1300::DischargeCurrentGet() {
  uint16_t iset;
  npmx_charger_discharging_current_get(npmx_charger_get(&_npmx_instance, 0), &iset);
  return iset;
}
void hal::pmic::esp32s3::PmicNpm1300::ChargeVtermSet(uint16_t vterm) {
  npmx_charger_termination_normal_voltage_set(npmx_charger_get(&_npmx_instance, 0),
                                              npmx_charger_voltage_convert(vterm));
  /* Set battery termination voltage in warm temperature. */
  npmx_charger_termination_warm_voltage_set(npmx_charger_get(&_npmx_instance, 0),
                                            npmx_charger_voltage_convert(vterm));
}
uint16_t hal::pmic::esp32s3::PmicNpm1300::ChargeVtermGet() {
  npmx_charger_voltage_t voltage_enum;
  npmx_charger_termination_normal_voltage_get(npmx_charger_get(&_npmx_instance, 0), &voltage_enum);
  uint32_t voltage;
  npmx_charger_voltage_convert_to_mv(voltage_enum, &voltage);
  return static_cast<uint16_t>(voltage);
}

hal::charger::Charger::ChargeStatus hal::pmic::esp32s3::PmicNpm1300::ChargeStatusGet() {
  if (_charge_error != Charger::ChargeError::NONE) {
    return Charger::ChargeStatus::ERROR;
  }
  uint8_t status_mask;
  npmx_charger_status_get(npmx_charger_get(&_npmx_instance, 0), &status_mask);
  LOGI(TAG, "Charger status mask %u", status_mask);
//  if (NPMX_CHARGER_STATUS_SUPPLEMENT_ACTIVE_MASK & status_mask) {
//    return Charger::ChargeStatus::SUPPLEMENTACTIVE;
//  }
  if (NPMX_CHARGER_STATUS_TRICKLE_CHARGE_MASK & status_mask) {
    return Charger::ChargeStatus::TRICKLECHARGE;
  }
  if (NPMX_CHARGER_STATUS_CONSTANT_CURRENT_MASK & status_mask) {
    return Charger::ChargeStatus::CONSTANTCURRENT;
  }
  if (NPMX_CHARGER_STATUS_CONSTANT_VOLTAGE_MASK & status_mask) {
    return Charger::ChargeStatus::CONSTANTVOLTAGE;
  }
  if (NPMX_CHARGER_STATUS_RECHARGE_MASK & status_mask) {
    return Charger::ChargeStatus::CONSTANTVOLTAGE;
  }
  if (NPMX_CHARGER_STATUS_DIE_TEMP_HIGH_MASK & status_mask) {
    return Charger::ChargeStatus::PAUSED;
  }
  if (NPMX_CHARGER_STATUS_COMPLETED_MASK & status_mask) {
    return Charger::ChargeStatus::COMPLETED;
  }
  return Charger::ChargeStatus::NONE;
}
hal::charger::Charger::ChargeError hal::pmic::esp32s3::PmicNpm1300::ChargeErrorGet() {
  return _charge_error;
}
bool hal::pmic::esp32s3::PmicNpm1300::ChargeBattDetect() {
  uint8_t status_mask;
  npmx_charger_status_get(npmx_charger_get(&_npmx_instance, 0), &status_mask);
  return NPMX_CHARGER_STATUS_BATTERY_DETECTED_MASK & status_mask;
}
void hal::pmic::esp32s3::PmicNpm1300::HandleInterrupt() {
  npmx_core_interrupt(&_npmx_instance);
}
void hal::pmic::esp32s3::PmicNpm1300::Loop() {
  npmx_core_proc(&_npmx_instance);
}
Boards::WakeupSource hal::pmic::esp32s3::PmicNpm1300::GetWakeup() {
  return _wakeup_source;
}

void hal::pmic::esp32s3::PmicNpm1300::PwrLedSet(hal::gpio::Gpio *gpio) {
  _power_led = gpio;
  uint8_t status;
  npmx_vbusin_vbus_status_get(npmx_vbusin_get(&_npmx_instance, 0), &status);
  if(_power_led != nullptr){
    _power_led->GpioWrite((status&NPMX_VBUSIN_STATUS_CONNECTED_MASK)==1);
  }
}

void hal::pmic::esp32s3::PmicNpm1300::Init() {
  cc_set_current(&_npmx_instance);
  npmx_vbusin_suspend_mode_enable_set(npmx_vbusin_get(&_npmx_instance, 0), false);

  _wakeup_source = ::get_wakeup(&_npmx_instance);

  gpio_config_t int_gpio = {};

  int_gpio.intr_type = GPIO_INTR_ANYEDGE;
  int_gpio.mode = GPIO_MODE_INPUT;
  int_gpio.pin_bit_mask = (1ULL << _gpio_int);
  int_gpio.pull_down_en = GPIO_PULLDOWN_DISABLE;
  int_gpio.pull_up_en = GPIO_PULLUP_DISABLE;
  gpio_config(&int_gpio);

  gpio_isr_handler_add(_gpio_int, irq, this);
  gpio_set_intr_type(_gpio_int, GPIO_INTR_POSEDGE);

  const esp_timer_create_args_t npmx_loop_timer = {
      .callback = &npmx_timer,
      .arg = this,
      .dispatch_method = ESP_TIMER_TASK,
      .name = "npmx",
      .skip_unhandled_events = true
  };

  ESP_ERROR_CHECK(esp_timer_create(&npmx_loop_timer, &_looptimer));
  ESP_ERROR_CHECK(esp_timer_start_periodic(_looptimer, 500*1000));

  npmx_core_event_interrupt_enable(&_npmx_instance, NPMX_EVENT_GROUP_VBUSIN_VOLTAGE,
                                   NPMX_EVENT_GROUP_VBUSIN_DETECTED_MASK |
                                       NPMX_EVENT_GROUP_VBUSIN_REMOVED_MASK);
  npmx_core_register_cb(&_npmx_instance, VbusVoltage, NPMX_CALLBACK_TYPE_EVENT_VBUSIN_VOLTAGE);
  npmx_core_event_interrupt_enable(&_npmx_instance, NPMX_EVENT_GROUP_SHIPHOLD,
                                   NPMX_EVENT_GROUP_SHIPHOLD_PRESSED_MASK);
//  npmx_core_event_interrupt_enable(&_npmx_instance, NPMX_EVENT_GROUP_BAT_CHAR_BAT,
//                                   NPMX_EVENT_GROUP_BATTERY_DETECTED_MASK |
//                                       NPMX_EVENT_GROUP_BATTERY_REMOVED_MASK);
  npmx_core_event_interrupt_enable(&_npmx_instance, NPMX_EVENT_GROUP_VBUSIN_THERMAL,
                                   NPMX_EVENT_GROUP_USB_CC1_MASK |
                                       NPMX_EVENT_GROUP_USB_CC2_MASK);
  npmx_core_register_cb(&_npmx_instance, vbus_thermal, NPMX_CALLBACK_TYPE_EVENT_VBUSIN_THERMAL_USB);

  npmx_core_event_interrupt_enable(&_npmx_instance, NPMX_EVENT_GROUP_BAT_CHAR_STATUS,
                                   NPMX_EVENT_GROUP_CHARGER_COMPLETED_MASK |
                                       NPMX_EVENT_GROUP_CHARGER_ERROR_MASK);

  npmx_core_register_cb(&_npmx_instance, GpioHandler, NPMX_CALLBACK_TYPE_EVENT_EVENTSGPIOSET);

}

void hal::pmic::esp32s3::PmicNpm1300::EnableGpioEvent(uint8_t index) {
  _gpio_event_mask |= (1 << index);
  npmx_core_event_interrupt_enable(&_npmx_instance, NPMX_EVENT_GROUP_GPIO, _gpio_event_mask);
}

void hal::pmic::esp32s3::PmicNpm1300::DisableGpioEvent(uint8_t index) {
  _gpio_event_mask &= ~(1 << index);
  npmx_core_event_interrupt_enable(&_npmx_instance, NPMX_EVENT_GROUP_GPIO, _gpio_event_mask);
}


void hal::pmic::esp32s3::PmicNpm1300::GpioHandler(npmx_instance_t *pm, npmx_callback_type_t, uint8_t mask) {
  auto *pmic = static_cast<hal::pmic::esp32s3::PmicNpm1300 *>(npmx_core_context_get(pm));
  for(int x = 0; x< 5; x++){
    if(mask & (1 << x)){
      LOGI(TAG, "GPIO %u", x);
      pmic->GpioCallback(x);
    }
  }

}
void hal::pmic::esp32s3::PmicNpm1300::GpioCallback(uint8_t index) {
  if(_gpio_callbacks[index] != nullptr){
    _gpio_callbacks[index]();
  }
}
void hal::pmic::esp32s3::PmicNpm1300::RegisterGpioCallback(uint8_t index, void (*callback)()) {
  _gpio_callbacks[index] = callback;
}
uint16_t hal::pmic::esp32s3::PmicNpm1300::VbusMaxCurrentGet() {
  npmx_vbusin_cc_t cc1;
  npmx_vbusin_cc_t cc2;
  npmx_vbusin_cc_status_get(npmx_vbusin_get(&_npmx_instance, 0), &cc1, &cc2);
  switch(cc1 >= cc2 ? cc1 : cc2){
    case NPMX_VBUSIN_CC_NOT_CONNECTED:
      return 0;
      break;
    case NPMX_VBUSIN_CC_DEFAULT:
      return 500;
      break;
    case NPMX_VBUSIN_CC_HIGH_POWER_1A5:
      return 1500;
      break;
    case NPMX_VBUSIN_CC_HIGH_POWER_3A0:
      return 3000;
      break;
    default:
      return 0;
  }
}

void hal::pmic::esp32s3::PmicNpm1300::VbusMaxCurrentSet(uint16_t mA) {
  npmx_vbusin_current_t setting = npmx_vbusin_current_convert(((mA+ 499) / 500) * 500);
  npmx_vbusin_current_limit_set(npmx_vbusin_get(&_npmx_instance, 0), setting);
}

bool hal::pmic::esp32s3::PmicNpm1300::VbusConnected() {
  uint8_t vbus_status;
  npmx_vbusin_vbus_status_get(npmx_vbusin_get(&_npmx_instance, 0), &vbus_status);
  return vbus_status&NPMX_VBUSIN_STATUS_CONNECTED_MASK;
}

void hal::pmic::esp32s3::PmicNpm1300::EnableADC() {
  npmx_adc_ntc_config_t ntc_config = { .type = NPMX_ADC_NTC_TYPE_10_K, .beta = 3380 };

  /* Set thermistor type and NTC beta value for ADC measurements. */
  npmx_adc_ntc_config_set(npmx_adc_get(&_npmx_instance, 0), &ntc_config);

  /* Enable auto measurement of the battery current after the battery voltage measurement. */
  npmx_adc_ibat_meas_enable_set(npmx_adc_get(&_npmx_instance, 0), true);

  const esp_timer_create_args_t kAdcTimer = {
      .callback = &ADCTimerHandler,
      .arg = this,
      .dispatch_method = ESP_TIMER_TASK,
      .name = "npmx adc",
      .skip_unhandled_events = true
  };

  ESP_ERROR_CHECK(esp_timer_create(&kAdcTimer, &_adc_timer));
  ESP_ERROR_CHECK(esp_timer_start_periodic(_adc_timer, 1000*1000));
  if (npmx_adc_task_trigger(npmx_adc_get(&_npmx_instance, 0), NPMX_ADC_TASK_SINGLE_SHOT_VBAT) != NPMX_SUCCESS) {
    LOGI(TAG, "Triggering VBAT measurement failed.");
  }
  if (npmx_adc_task_trigger(npmx_adc_get(&_npmx_instance, 0), NPMX_ADC_TASK_SINGLE_SHOT_NTC) != NPMX_SUCCESS) {
    LOGI(TAG, "Triggering NTC measurement failed.");
  }

}

void hal::pmic::esp32s3::PmicNpm1300::ADCTimerHandler(void *arg) {
  auto pmic = static_cast<hal::pmic::esp32s3::PmicNpm1300 *>(arg);
  npmx_adc_meas_all_t meas;
  if (npmx_adc_meas_all_get(npmx_adc_get(&pmic->_npmx_instance, 0), &meas) != NPMX_SUCCESS) {
    LOGI(TAG, "Reading ADC measurements failed.");
  }
  /* Convert voltage in millivolts to voltage in volts. */
  pmic->_vbat = meas.values[NPMX_ADC_MEAS_VBAT];
  /* Convert current in milliamperes to current in amperes. */
  pmic->_ibat = meas.values[NPMX_ADC_MEAS_VBAT2_IBAT];
  /* Convert temperature in millidegrees Celsius to temperature in Celsius */
  pmic->_tbat = meas.values[NPMX_ADC_MEAS_BAT_TEMP];
  if (npmx_adc_task_trigger(npmx_adc_get(&pmic->_npmx_instance, 0), NPMX_ADC_TASK_SINGLE_SHOT_VBAT) != NPMX_SUCCESS) {
    LOGI(TAG, "Triggering VBAT measurement failed.");
  }
  if (npmx_adc_task_trigger(npmx_adc_get(&pmic->_npmx_instance, 0), NPMX_ADC_TASK_SINGLE_SHOT_NTC) != NPMX_SUCCESS) {
    LOGI(TAG, "Triggering NTC measurement failed.");
  }

}
double hal::pmic::esp32s3::PmicNpm1300::BatteryCurrent() {
  return _ibat/1000.00;
}
double hal::pmic::esp32s3::PmicNpm1300::BatteryTemperature() {
  return _tbat/1000.00;
}
void hal::pmic::esp32s3::PmicNpm1300::VbusConnectedCallback(void (*callback)(bool)) {
  _vbus_callback = callback;
  uint8_t status;
  npmx_vbusin_vbus_status_get(npmx_vbusin_get(&_npmx_instance, 0), &status);
  if(_vbus_callback){
    _vbus_callback((status&NPMX_VBUSIN_STATUS_CONNECTED_MASK)==1);
  }
}
void  hal::pmic::esp32s3::PmicNpm1300::DebugLog() {
  LOGI(TAG, "SOC: %i, Voltage %fV", BatterySoc(), BatteryVoltage());
  LOGI(TAG, "Temperature: %fC, Current %fA", BatteryTemperature(), BatteryCurrent());
  uint8_t status_mask;
  npmx_charger_status_get(npmx_charger_get(&_npmx_instance, 0), &status_mask);
  LOGI(TAG, "State: %x", status_mask);
  switch (BatteryStatus()) {
    case State::OK:
      LOGI(TAG, "OK");
      break;
    case State::CHARGING:
      LOGI(TAG, "Charging");
      break;
    case State::CONNECTED_NOT_CHARGING:
      LOGI(TAG, "Connected not charging");
      break;
    case State::DISCHARGING:
      LOGI(TAG, "Discharging");
      break;
    case State::NOT_PRESENT:
      LOGI(TAG, "No Battery");
      break;
    case State::ERROR:
      LOGI(TAG, "Error");
      break;
  }
}


void hal::pmic::esp32s3::PmicNpm1300Gpio::GpioConfig(gpio::GpioDirection direction, gpio::GpioPullMode pull) {
  npmx_gpio_pull_t pull_mode = NPMX_GPIO_PULL_DOWN;
  switch (pull) {
    case gpio::GpioPullMode::NONE:
      pull_mode = NPMX_GPIO_PULL_NONE;
      break;
    case gpio::GpioPullMode::UP:
      pull_mode = NPMX_GPIO_PULL_UP;
      break;
    case gpio::GpioPullMode::DOWN:
      pull_mode = NPMX_GPIO_PULL_DOWN;
      break;
  }
  _config = {
      .mode = direction == gpio::GpioDirection::IN ? NPMX_GPIO_MODE_INPUT
                                             : NPMX_GPIO_MODE_OUTPUT_OVERRIDE_0,
      .drive = NPMX_GPIO_DRIVE_6_MA,
      .pull = pull_mode,
      .open_drain = false,
      .debounce = false,
  };
  npmx_gpio_config_set(npmx_gpio_get(_npmx_instance, _index), &_config);
}

bool hal::pmic::esp32s3::PmicNpm1300Gpio::GpioRead() {
  bool state;
  npmx_gpio_status_check(npmx_gpio_get(_npmx_instance, _index), &state);
  return state;
}

void hal::pmic::esp32s3::PmicNpm1300Gpio::GpioWrite(bool b) {
  npmx_gpio_mode_set(npmx_gpio_get(_npmx_instance, _index),
                     b ? NPMX_GPIO_MODE_OUTPUT_OVERRIDE_1 : NPMX_GPIO_MODE_OUTPUT_OVERRIDE_0);
}

hal::pmic::esp32s3::PmicNpm1300Gpio::PmicNpm1300Gpio(npmx_instance_t *npmx, uint8_t index) : _npmx_instance(npmx), _index(index) {

}

/***
 * Enables IRQ output on this GPIO
 * @param b true to enable, false to disable
 */
void hal::pmic::esp32s3::PmicNpm1300Gpio::EnableIrq(bool b) {
  npmx_gpio_mode_set(npmx_gpio_get(_npmx_instance, _index),
                     b ? NPMX_GPIO_MODE_OUTPUT_IRQ : NPMX_GPIO_MODE_INPUT);

}
void hal::pmic::esp32s3::PmicNpm1300Gpio::GpioInt(hal::gpio::GpioIntDirection dir, void (*callback)()) {
  _callback = callback;
  _int_direction = dir;
  npmx_gpio_mode_t set_dir = NPMX_GPIO_MODE_INPUT;
  switch(dir){
    case gpio::GpioIntDirection::NONE:
      set_dir = NPMX_GPIO_MODE_INPUT;
      break;
    case gpio::GpioIntDirection::RISING:
      set_dir = NPMX_GPIO_MODE_INPUT_RISING_EDGE;
      break;
    case gpio::GpioIntDirection::FALLING:
      set_dir = NPMX_GPIO_MODE_INPUT_FALLING_EDGE;
      break;
  }
  npmx_gpio_mode_set(npmx_gpio_get(_npmx_instance, _index), set_dir);
  auto pmic = static_cast<hal::pmic::esp32s3::PmicNpm1300 *>(npmx_core_context_get(_npmx_instance));
  if(dir != gpio::GpioIntDirection::NONE){
    pmic->EnableGpioEvent(_index);
  }
  else{
    pmic->DisableGpioEvent(_index);
  }
  pmic->RegisterGpioCallback(_index, callback);
}

void hal::pmic::esp32s3::PmicNpm1300ShpHld::GpioConfig(gpio::GpioDirection direction, gpio::GpioPullMode pull) {

}
bool hal::pmic::esp32s3::PmicNpm1300ShpHld::GpioRead() {
  bool state;
  npmx_ship_gpio_status_check(npmx_ship_get(_npmx_instance, 0), &state);
  return !state;
}
void hal::pmic::esp32s3::PmicNpm1300ShpHld::GpioWrite(bool b) {

}
hal::pmic::esp32s3::PmicNpm1300ShpHld::PmicNpm1300ShpHld(npmx_instance_t *npmx) : _npmx_instance(npmx) {

}
hal::pmic::esp32s3::PmicNpm1300Led::PmicNpm1300Led(npmx_instance_t *npmx, uint8_t index) : _npmx_instance(npmx), _index(index) {

}
void hal::pmic::esp32s3::PmicNpm1300Led::GpioConfig(gpio::GpioDirection direction, gpio::GpioPullMode pull) {

}
/***
 * Always returns false, as LED does not support input
 * @return false
 */
bool hal::pmic::esp32s3::PmicNpm1300Led::GpioRead() {
  return false;
}

/***
 * Write LED State
 * @param b true turns on the LED, false turns off
 */
void hal::pmic::esp32s3::PmicNpm1300Led::GpioWrite(bool b) {
  npmx_led_state_set(npmx_led_get(_npmx_instance, _index), b);
}

/***
 * Sets the LED to indicate charging status
 * @param b true enables, false disables
 */
void hal::pmic::esp32s3::PmicNpm1300Led::ChargingIndicator(bool b) {
  npmx_led_mode_set(npmx_led_get(_npmx_instance, _index), b ? NPMX_LED_MODE_CHARGING : NPMX_LED_MODE_HOST);
}

