#pragma once

#include <memory>

#include <hal/battery.h>
#include "i2c.h"
#include "hal/keys.h"
#include "npmx_backend.h"
#include "npmx_instance.h"
#include "hal/gpio.h"

class mfd_npm1300;

class mfd_npm1300_gpio: public Gpio {
 public:
  mfd_npm1300_gpio(npmx_instance_t *npmx, uint8_t index);
  void config(gpio_direction direction, gpio_pull_mode pull) override;
  bool read() override;
  void write(bool b) override;

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

};

class mfd_npm1300_shphld: public Gpio {
 public:
  mfd_npm1300_shphld(npmx_instance_t *npmx);
  void config(gpio_direction direction, gpio_pull_mode pull) override;
  bool read() override;
  void write(bool b) override;

 private:
  npmx_instance_t *_npmx_instance;
};


class mfd_npm1300 final : public Battery, Keys {
    public:
    explicit mfd_npm1300(I2C *, gpio_num_t gpio_int);
    ~mfd_npm1300() final = default;

    //Buck regulators
    void buck1_set(uint32_t millivolts);
    void buck1_disable();

    //Load Switch
    void loadsw1_enable();
    void loadsw1_disable();

    //GPIO
    mfd_npm1300_gpio *gpio_get(uint8_t index);
    mfd_npm1300_shphld *shphld_get();

  void poll();

    int pollInterval() override { return 10000; };

    double getVoltage() override;

    int getSoc() override;

    double getRate() override;

    void removed() override;

    void inserted() override;

    State status() override;

  EVENT_STATE * read() override;

    private:
    I2C *_i2c;
    double _voltage = 0;
    int _soc = 0;
    double _rate = 0;
    gpio_num_t _gpio_int;
    bool present = false;

//NPM1300 Stuff
  npmx_backend_t _npmx_backend;
  npmx_instance_t _npmx_instance;
};