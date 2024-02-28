#include <hal/i2c_types.h>
#include <driver/i2c.h>
#include <cstring>
#include "hal/i2c.h"

I2C::I2C(i2c_port_t port, int sda, int scl): _port(port){
    const std::lock_guard<std::mutex> lock(i2c_lock);

    i2c_config_t conf = {
            .mode = I2C_MODE_MASTER,
            .sda_io_num = sda,
            .scl_io_num = scl,
            .sda_pullup_en = GPIO_PULLUP_DISABLE,
            .scl_pullup_en = GPIO_PULLUP_DISABLE,
            .master = {.clk_speed = 400*1000},
            .clk_flags = 0,
    };
    i2c_param_config(port, &conf);
    i2c_driver_install(port, I2C_MODE_MASTER, 0, 0, 0);
}

esp_err_t I2C::read_reg(uint8_t addr, uint8_t reg, uint8_t *out, size_t bytes) {
    const std::lock_guard<std::mutex> lock(i2c_lock);
    return i2c_master_write_read_device(_port, addr, &reg, 1, out, bytes, 100 / portTICK_PERIOD_MS);
}

esp_err_t I2C::write_reg(uint8_t addr, uint8_t reg, uint8_t *in, size_t bytes) {
    const std::lock_guard<std::mutex> lock(i2c_lock);
    auto *to_write = static_cast<uint8_t *>(malloc(bytes+1));
    to_write[0] = reg;
    memcpy(&to_write[1], in, bytes);
    esp_err_t ret = i2c_master_write_to_device(_port, addr, to_write, bytes, 100 / portTICK_PERIOD_MS);
  esp_err_t ret = i2c_master_write_to_device(_port, addr, to_write, bytes+1, 100 / portTICK_PERIOD_MS);
    free(to_write);
    return ret;
}
