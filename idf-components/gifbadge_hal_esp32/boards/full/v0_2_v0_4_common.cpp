#include "boards/full/v0_2_v0_4_common.h"

#include <esp_pm.h>
#include "log.h"
#include <driver/sdmmc_defs.h>
#include "esp_io_expander.h"
#include "drivers/keys_esp_io_expander.h"
#include "esp_io_expander_tca95xx_16bit.h"

#define USB_ENABLE

static const char *TAG = "board_2_1_v0_2v0_4";

static bool checkSdState(esp_io_expander_handle_t io_expander) {
  uint32_t levels;
  esp_io_expander_get_level(io_expander, 0xffff, &levels);
  if (levels & (IO_EXPANDER_PIN_NUM_15)) {
    return false;
  }
  return true;
}

static bool sdState;

static void checkSDTimer(void *arg) {
  auto io_expander = (esp_io_expander_handle_t) arg;
  if (checkSdState(io_expander) != sdState) {
    esp_restart();
  }
}

namespace Boards {

esp32::s3::full::v0_2v0_4::v0_2v0_4() {

}

hal::battery::Battery *esp32::s3::full::v0_2v0_4::GetBattery() {
  return _battery;
}

hal::touch::Touch *esp32::s3::full::v0_2v0_4::GetTouch() {
  return _touch;
}

hal::keys::Keys *esp32::s3::full::v0_2v0_4::GetKeys() {
  return _keys;
}

hal::display::Display *esp32::s3::full::v0_2v0_4::GetDisplay() {
  return _display;
}

hal::backlight::Backlight *esp32::s3::full::v0_2v0_4::GetBacklight() {
  return _backlight;
}

void esp32::s3::full::v0_2v0_4::PowerOff() {
  LOGI(TAG, "Poweroff");
  esp32s3_sdmmc::PowerOff();
  vTaskDelay(100 / portTICK_PERIOD_MS);
  esp_io_expander_set_level(_io_expander, IO_EXPANDER_PIN_NUM_3, 1);

}

BoardPower esp32::s3::full::v0_2v0_4::PowerState() {
  if (_vbus->VbusConnected()) {
    return BOARD_POWER_NORMAL;
  }
  if (_battery->BatterySoc() < 12) {
    if (_battery->BatterySoc() < 10) {
      return BOARD_POWER_CRITICAL;
    }
    return BOARD_POWER_LOW;

  }
  return BOARD_POWER_NORMAL;
}

bool esp32::s3::full::v0_2v0_4::StorageReady() {
  return checkSdState(_io_expander);
}

//Work Around for broken USB connection detection
void vbusISR(void *) {
  esp32::s3::esp32s3::VbusHandler(gpio_get_level(GPIO_NUM_0));
}

void esp32::s3::full::v0_2v0_4::LateInit() {
 buffer = heap_caps_malloc(480 * 480 + 0x6100, MALLOC_CAP_INTERNAL);
  _config = new hal::config::esp32s3::Config_NVS();
  _i2c = new I2C(I2C_NUM_0, 47, 48, 100 * 1000, false);
  _battery = new hal::battery::esp32s3::battery_max17048(_i2c, GPIO_NUM_0);
  /*G3, G4, G5, R1, R2, R3, R4, R5, B1, B2, B3, B4, B5, G0, G1, G2 */
  std::array<int, 16> rgb = {11, 12, 13, 3, 4, 5, 6, 7, 14, 15, 16, 17, 18, 8, 9, 10};
//    uint8_t data = IO_EXPANDER_PIN_NUM_3;
//    _i2c->write_reg(ESP_IO_EXPANDER_I2C_TCA9555_ADDRESS_000, 0x04, &data, 1);
  esp_io_expander_new_i2c_tca95xx_16bit(I2C_NUM_0, ESP_IO_EXPANDER_I2C_TCA9555_ADDRESS_000, &_io_expander);
  //Turn of the shutdown pin output on the IO Expander before setting the direction to output.
  //Failure to do so results in turning the device off
  uint8_t data = 0xF7;
  _i2c->write_reg(ESP_IO_EXPANDER_I2C_TCA9555_ADDRESS_000, 0x02, &data, 1);
  esp_io_expander_set_dir(_io_expander, IO_EXPANDER_PIN_NUM_3, IO_EXPANDER_OUTPUT);
  esp_io_expander_set_level(_io_expander, IO_EXPANDER_PIN_NUM_3, 0);

  esp_io_expander_set_dir(_io_expander, IO_EXPANDER_PIN_NUM_2, IO_EXPANDER_OUTPUT);
  esp_io_expander_set_level(_io_expander, IO_EXPANDER_PIN_NUM_2, 0);
  vTaskDelay(pdMS_TO_TICKS(10));
  esp_io_expander_set_level(_io_expander, IO_EXPANDER_PIN_NUM_2, 1);

  spi_line_config_t line_config = {
      .cs_io_type = IO_TYPE_EXPANDER,             // Set to `IO_TYPE_GPIO` if using GPIO, same to below
      .cs_gpio_num = IO_EXPANDER_PIN_NUM_1,
      .scl_io_type = IO_TYPE_GPIO,
      .scl_gpio_num = 3,
      .sda_io_type = IO_TYPE_GPIO,
      .sda_gpio_num = 4,
      .io_expander = _io_expander,                        // Set to NULL if not using IO expander
  };
  _display = new hal::display::esp32s3::display_st7701s(line_config, 2, 1, 45, 46, rgb);

  esp_io_expander_set_dir(_io_expander,
                          IO_EXPANDER_PIN_NUM_14 | IO_EXPANDER_PIN_NUM_12 | IO_EXPANDER_PIN_NUM_13,
                          IO_EXPANDER_INPUT);
  esp_io_expander_print_state(_io_expander);
  _keys = new hal::keys::esp32s3::keys_esp_io_expander(_io_expander, _i2c, 14, 12, 13);

  _backlight = new hal::backlight::esp32s3::backlight_ledc(GPIO_NUM_21, false, 0);
  _backlight->setLevel(_config->getBacklight() * 10);
  _touch = new hal::touch::esp32s3::touch_ft5x06(_i2c);

  //TODO: Check if we can use DFS with the RGB LCD
  esp_pm_config_t pm_config = {.max_freq_mhz = 240, .min_freq_mhz = 80, .light_sleep_enable = false};
  esp_pm_configure(&pm_config);

  gpio_config_t vbus_config = {};

  vbus_config.intr_type = GPIO_INTR_ANYEDGE;
  vbus_config.mode = GPIO_MODE_INPUT;
  vbus_config.pin_bit_mask = (1ULL << 0);
  vbus_config.pull_down_en = GPIO_PULLDOWN_DISABLE;
  vbus_config.pull_up_en = GPIO_PULLUP_DISABLE;
  gpio_config(&vbus_config);

  if (checkSdState(_io_expander)) {
    mount(GPIO_NUM_40, GPIO_NUM_41, GPIO_NUM_39, GPIO_NUM_38, GPIO_NUM_44, GPIO_NUM_42, GPIO_NUM_NC, 4, GPIO_NUM_NC);
  }

  sdState = checkSdState(_io_expander);
  const esp_timer_create_args_t checkSdTimerArgs = {
      .callback = &checkSDTimer,
      .arg = _io_expander,
      .dispatch_method = ESP_TIMER_TASK,
      .name = "sdcard_check",
      .skip_unhandled_events = true
  };
  esp_timer_handle_t sdTimer = nullptr;
  ESP_ERROR_CHECK(esp_timer_create(&checkSdTimerArgs, &sdTimer));
  ESP_ERROR_CHECK(esp_timer_start_periodic(sdTimer, 500 * 1000));
  _vbus = new hal::vbus::esp32s3::b2_1_v0_2v0_4_vbus(GPIO_NUM_0, _io_expander, 8);

  //Work Around for broken USB connection detection
  gpio_install_isr_service(ESP_INTR_FLAG_EDGE);
  gpio_isr_handler_add(GPIO_NUM_0, vbusISR, this);
  vbusISR(nullptr);
}
hal::vbus::Vbus *esp32::s3::full::v0_2v0_4::GetVbus() {
  return _vbus;
}
}
uint16_t hal::vbus::esp32s3::b2_1_v0_2v0_4_vbus::VbusMaxCurrentGet() {
  if(gpio_get_level(_gpio)){
    uint32_t levels;
    if(esp_io_expander_get_level(_io_expander, 0xffff, &levels) == ESP_OK){
      if(levels  & (1 << _expander_gpio)){
        return 500;
      }
      else {
        return 1500;
      }
    }
    else {
      return 500;
    }
  }
  return 0;
}

void hal::vbus::esp32s3::b2_1_v0_2v0_4_vbus::VbusMaxCurrentSet(uint16_t mA) {

}

bool hal::vbus::esp32s3::b2_1_v0_2v0_4_vbus::VbusConnected() {
  return gpio_get_level(_gpio);
}

hal::vbus::esp32s3::b2_1_v0_2v0_4_vbus::b2_1_v0_2v0_4_vbus(gpio_num_t gpio, esp_io_expander_handle_t expander, uint8_t expander_pin): _io_expander(expander), _gpio(gpio), _expander_gpio(expander_pin) {


}
