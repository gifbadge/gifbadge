#pragma once

#include <memory>

#include <hal/battery.h>
#include <esp_timer.h>
#include "i2c.h"
#include "hal/keys.h"
#include "npmx_backend.h"
#include "npmx_instance.h"
#include "hal/gpio.h"
#include "hal/charger.h"
#include "hal/board.h"
#include "hal/vbus.h"

class PmicNpm1300;

class PmicNpm1300Gpio final : public Gpio {
 public:
  PmicNpm1300Gpio(npmx_instance_t *npmx, uint8_t index);
  ~PmicNpm1300Gpio() = default;
  void GpioConfig(GpioDirection direction, GpioPullMode pull) override;
  bool GpioRead() override;
  void GpioWrite(bool b) override;
  void EnableIrq(bool b);
  void GpioInt(GpioIntDirection dir, void (*callback)()) override;

 private:
  npmx_instance_t *_npmx_instance;
  npmx_gpio_config_t _config{
      .mode = NPMX_GPIO_MODE_INPUT,
      .drive = NPMX_GPIO_DRIVE_6_MA,
      .pull = NPMX_GPIO_PULL_DOWN,
      .open_drain = false,
      .debounce = false,
  };
  uint8_t _index;
  GpioIntDirection _int_direction = GpioIntDirection::NONE;
  void (*_callback)() = nullptr;
};

class PmicNpm1300ShpHld : public Gpio {
 public:
  explicit PmicNpm1300ShpHld(npmx_instance_t *npmx);
  void GpioConfig(GpioDirection direction, GpioPullMode pull) override;
  bool GpioRead() override;
  void GpioWrite(bool b) override;

 private:
  npmx_instance_t *_npmx_instance;
};

class PmicNpm1300Led : public Gpio {
 public:
  PmicNpm1300Led(npmx_instance_t *npmx, uint8_t index);
  void GpioConfig(GpioDirection direction, GpioPullMode pull) override;
  bool GpioRead() override;
  void GpioWrite(bool b) override;
  void ChargingIndicator(bool b);

 private:
  npmx_instance_t *_npmx_instance;
  uint8_t _index;
};

class PmicNpm1300 final : public Battery, public Charger, public Vbus {
 public:
  explicit PmicNpm1300(I2C *, gpio_num_t gpio_int);
  ~PmicNpm1300() final = default;

  void Init();

  //Buck regulators
  void Buck1Set(uint32_t millivolts);
  void Buck1Disable();

  //Load Switch
  void LoadSw1Enable();
  void LoadSw1Disable();

  //GPIO
  PmicNpm1300Gpio *GpioGet(uint8_t index);
  PmicNpm1300ShpHld *ShphldGet();
  PmicNpm1300Led *LedGet(uint8_t index);

  void poll();

  int pollInterval() override { return 10000; };

  double BatteryVoltage() override;
  double BatteryCurrent() override;
  double BatteryTemperature() override;

  int getSoc() override;

  double getRate() override;

  void removed() override;

  void inserted() override;

  State status() override;

  void ChargeEnable() override;
  void ChargeDisable() override;
  void ChargeCurrentSet(uint16_t iset) override;
  uint16_t ChargeCurrentGet() override;
  void DischargeCurrentSet(uint16_t iset) override;
  uint16_t DischargeCurrentGet() override;
  void ChargeVtermSet(uint16_t vterm) override;
  uint16_t ChargeVtermGet() override;
  Charger::ChargeStatus ChargeStatusGet() override;
  ChargeError ChargeErrorGet() override;
  bool ChargeBattDetect() override;

  void HandleInterrupt();
  void Loop();

  Boards::Board::WAKEUP_SOURCE GetWakeup();

  void PwrLedSet(Gpio *gpio);

  void EnableGpioEvent(uint8_t index);
  void DisableGpioEvent(uint8_t index);
  void RegisterGpioCallback(uint8_t index, void (*callback)());

  uint16_t VbusMaxCurrentGet() override;
  void VbusMaxCurrentSet(uint16_t mA) override;
  bool VbusConnected() override;
 private:
  I2C *_i2c;
  double _voltage = 0;
  int _soc = 0;
  double _rate = 0;
  gpio_num_t _gpio_int;
  bool _present = false;
  Boards::Board::WAKEUP_SOURCE _wakeup_source = Boards::Board::WAKEUP_SOURCE::NONE;
  Gpio *_power_led = nullptr;

//NPM1300 Stuff
  npmx_backend_t _npmx_backend;
  npmx_instance_t _npmx_instance;

  Charger::ChargeError _charge_error = Charger::ChargeError::NONE;

  esp_timer_handle_t _looptimer = nullptr;

  static void VbusVoltage(npmx_instance_t *pm, npmx_callback_type_t type, uint8_t mask);
  static void GpioHandler(npmx_instance_t *pm, npmx_callback_type_t type, uint8_t mask);
  void GpioCallback(uint8_t index);

  uint8_t _gpio_event_mask;
  void (*_gpio_callbacks[5])() = {nullptr, nullptr, nullptr, nullptr, nullptr};

};