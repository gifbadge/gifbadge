#include "boards/b2_1_v0_5.h"

#include <esp_pm.h>
#include "log.h"
#include <driver/sdmmc_defs.h>
#include <driver/rtc_io.h>
#include <esp_sleep.h>
#include "esp_io_expander_cat9532.h"
#include "drivers/keys_esp_io_expander.h"
#include "drivers/esp_io_expander_gpio.h"

#define USB_ENABLE

static const char *TAG = "board_2_1_v0_5";

static bool checkSdState(Gpio *gpio) {
  return !gpio->GpioRead();
}

static bool sdState;

//static void checkSDTimer(void *arg) {
//  auto io_expander = (esp_io_expander_handle_t) arg;
//  if (checkSdState(io_expander) != sdState) {
//    esp_restart();
//  }
//}

namespace Boards {

b2_1_v0_5::b2_1_v0_5() {
  _config = new Config_NVS();
  gpio_install_isr_service(0);
  _i2c = new I2C(I2C_NUM_0, 47, 48, 100 * 1000, false);

  //Initialize the PMIC, and start the battery charging
  _pmic = new PmicNpm1300(_i2c, GPIO_NUM_21);

  //Enable Battery charging
  _pmic->ChargeCurrentSet(800);
  _pmic->ChargeVtermSet(4200);
  _pmic->ChargeEnable();

  //Enable charging LED
  _charge_led = _pmic->LedGet(1);
  _charge_led->ChargingIndicator(true);

  //Enable IRQ from npm1300
  PmicNpm1300Gpio *irq = _pmic->GpioGet(2);
  irq->EnableIrq(true);
  delete irq;

  //Set up the power LED
  _vbus_led = _pmic->LedGet(0);
  _vbus_led->ChargingIndicator(false);
  _pmic->PwrLedSet(_vbus_led);

  _pmic->Init();


  _battery = new battery_max17048(_i2c, GPIO_NUM_0);
}

Battery *b2_1_v0_5::getBattery() {
  return _battery;
}

Touch *b2_1_v0_5::getTouch() {
  return _touch;
}

Keys *b2_1_v0_5::getKeys() {
  return _keys;
}

Display *b2_1_v0_5::getDisplay() {
  return _display;
}

Backlight *b2_1_v0_5::getBacklight() {
  return _backlight;
}

void b2_1_v0_5::powerOff() {
  LOGI(TAG, "Poweroff");
  vTaskDelay(100 / portTICK_PERIOD_MS);
  rtc_gpio_pullup_dis(GPIO_NUM_21);
  rtc_gpio_pulldown_dis(GPIO_NUM_21);
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_21, 1);
  _pmic->LoadSw1Disable();
  _pmic->Buck1Disable();
  esp_deep_sleep_start();
}

BOARD_POWER b2_1_v0_5::powerState() {
//  if (powerConnected() != CHARGE_NONE) {
//    return BOARD_POWER_NORMAL;
//  }
//  if (_battery->getSoc() < 12) {
//    if (_battery->getSoc() < 10) {
//      return BOARD_POWER_CRITICAL;
//    }
//    return BOARD_POWER_LOW;
//
//  }
  return BOARD_POWER_NORMAL;
}

bool b2_1_v0_5::storageReady() {
  return checkSdState(_card_detect);
}

const char *b2_1_v0_5::name() {
  return "2.1\" 0.5";
}
void b2_1_v0_5::lateInit() {
  buffer = heap_caps_malloc(480 * 480 + 0x6100, MALLOC_CAP_INTERNAL);

  _pmic->Buck1Set(3300);

  _backlight = new backlight_ledc(GPIO_NUM_0, true, 0);
  _touch = new touch_ft5x06(_i2c);

  Gpio *_cs = _pmic->GpioGet(1);
  _cs->GpioConfig(Gpio::GpioDirection::OUT, Gpio::GpioPullMode::NONE);
  _cs->GpioWrite(true);

  esp_io_expander_new_gpio(_cs, &_io_expander);

  spi_line_config_t line_config = {
      .cs_io_type = IO_TYPE_EXPANDER,             // Set to `IO_TYPE_GPIO` if using GPIO, same to below
      .cs_gpio_num = 1,
      .scl_io_type = IO_TYPE_GPIO,
      .scl_gpio_num = 3,
      .sda_io_type = IO_TYPE_GPIO,
      .sda_gpio_num = 4,
      .io_expander = _io_expander,                        // Set to NULL if not using IO expander
  };


  PmicNpm1300Gpio *up = _pmic->GpioGet(4);
  up->GpioConfig(Gpio::GpioDirection::IN, Gpio::GpioPullMode::UP);

  PmicNpm1300Gpio *down = _pmic->GpioGet(3);
  down->GpioConfig(Gpio::GpioDirection::IN, Gpio::GpioPullMode::UP);

  PmicNpm1300ShpHld *enter = _pmic->ShphldGet();

  _keys = new KeysGeneric(up, down, enter);

  _pmic->LoadSw1Enable();
  _card_detect = _pmic->GpioGet(0);
  _card_detect->GpioConfig(Gpio::GpioDirection::IN, Gpio::GpioPullMode::UP);

  if (storageReady()) {
    _card_detect->GpioInt(Gpio::GpioIntDirection::RISING, esp_restart);
    mount(GPIO_NUM_40, GPIO_NUM_41, GPIO_NUM_39, GPIO_NUM_38, GPIO_NUM_44, GPIO_NUM_42, GPIO_NUM_NC, 4);
  }
  else {
    _card_detect->GpioInt(Gpio::GpioIntDirection::FALLING, esp_restart);
  }

  /*G3, G4, G5, R1, R2, R3, R4, R5, B1, B2, B3, B4, B5, G0, G1, G2 */
  std::array<int, 16> rgb = {11, 12, 13, 3, 4, 5, 6, 7, 14, 15, 16, 17, 18, 8, 9, 10};
  _display = new display_st7701s(line_config, 2, 1, 45, 46, rgb);


  //TODO: Check if we can use DFS with the RGB LCD
  esp_pm_config_t pm_config = {.max_freq_mhz = 240, .min_freq_mhz = 80, .light_sleep_enable = false};
  esp_pm_configure(&pm_config);

}
Board::WAKEUP_SOURCE b2_1_v0_5::bootReason() {
  if(esp_reset_reason() != ESP_RST_POWERON){
    return Board::WAKEUP_SOURCE::REBOOT;
  }
  return _pmic->GetWakeup();
}
Vbus *b2_1_v0_5::getVbus() {
  return _pmic;
}
Charger *b2_1_v0_5::getCharger() {
  return _pmic;
}
}