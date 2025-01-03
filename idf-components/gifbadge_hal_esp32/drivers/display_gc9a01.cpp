#include <driver/spi_master.h>
#include <esp_lcd_panel_vendor.h>
#include <esp_lcd_panel_ops.h>
#include <driver/gpio.h>
#include <esp_lcd_panel_io.h>
#include <cstring>
#include "log.h"
#include "esp_lcd_gc9a01.h"

#include "drivers/display_gc9a01.h"

static const char *TAG = "display_gc9a01.cpp";

hal::display::esp32s3::display_gc9a01::display_gc9a01(int mosi, int sck, int cs, int dc, int reset) {
  LOGI(TAG, "Initialize SPI bus");
  spi_bus_config_t buscfg = {
      .mosi_io_num = mosi,//21,
      .miso_io_num = -1,
      .sclk_io_num = sck,//40,
      .quadwp_io_num = -1,
      .quadhd_io_num = -1,
      .data4_io_num = -1,
      .data5_io_num = -1,
      .data6_io_num = -1,
      .data7_io_num = -1,
      .max_transfer_sz = 16384, //H_RES * V_RES * sizeof(uint16_t),
      .flags = 0,
      .isr_cpu_id = ESP_INTR_CPU_AFFINITY_AUTO,
      .intr_flags = 0,
  };
  ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO));

  LOGI(TAG, "Install panel IO");
  esp_lcd_panel_io_spi_config_t io_config = {
      .cs_gpio_num = cs, //47,
      .dc_gpio_num = dc,//45,
      .spi_mode = 0,
      .pclk_hz = (80 * 1000 * 1000),
      .trans_queue_depth = 10,
      .on_color_trans_done = nullptr,
      .user_ctx = nullptr,
      .lcd_cmd_bits = 8,
      .lcd_param_bits = 8,
      .flags = {
          .dc_high_on_cmd = 0,
          .dc_low_on_data = 0,
          .dc_low_on_param = 0,
          .octal_mode = 0,
          .quad_mode = 0,
          .sio_mode = 0,
          .lsb_first = 0,
          .cs_high_active = 0,
      }
  };
  // Attach the LCD to the SPI bus
  ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t) SPI2_HOST, &io_config, &io_handle));

  esp_lcd_panel_dev_config_t panel_config = {
      .reset_gpio_num = reset,
      .rgb_endian = LCD_RGB_ENDIAN_BGR,
      .data_endian = LCD_RGB_DATA_ENDIAN_BIG,
      .bits_per_pixel = 16,
      .flags = {
          .reset_active_high = 0,
      },
      .vendor_config = nullptr,
  };

  LOGI(TAG, "Install GC9A01 panel driver");
  ESP_ERROR_CHECK(esp_lcd_new_panel_gc9a01(io_handle, &panel_config, &panel_handle));
  ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
  ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
  ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panel_handle, true));
  ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel_handle, true, false));
  gpio_hold_en((gpio_num_t) reset); //Don't toggle the reset signal on light sleep
  ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));
  buffer = (uint8_t *) heap_caps_malloc(240 * 240 * 2, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  size = {240, 240};
}

static flushCallback_t pcallback = nullptr;

bool flush_ready(esp_lcd_panel_io_handle_t, esp_lcd_panel_io_event_data_t *, void *){
//  LOGI(TAG, "Flush");
  if(pcallback) {
    pcallback();
  }
  return false;
}

bool hal::display::esp32s3::display_gc9a01::onColorTransDone(flushCallback_t callback) {
  if (callback) {
    pcallback = callback;
    esp_lcd_panel_io_callbacks_t conf = {.on_color_trans_done = flush_ready};
    esp_lcd_panel_io_register_event_callbacks(io_handle, &conf, nullptr);
  } else {
    pcallback = nullptr;
    esp_lcd_panel_io_callbacks_t conf = {.on_color_trans_done = nullptr};
    esp_lcd_panel_io_register_event_callbacks(io_handle, &conf, nullptr);
  }
  return true;
}

void hal::display::esp32s3::display_gc9a01::write(int x_start, int y_start, int x_end, int y_end,
                                                  const void *color_data) {
  esp_lcd_panel_draw_bitmap(panel_handle, x_start, y_start, x_end, y_end, color_data);
}

void hal::display::esp32s3::display_gc9a01::clear() {
  memset(buffer, 0xFF, size.first * size.second * 2);
}