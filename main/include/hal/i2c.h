#pragma once
#include <hal/i2c_types.h>
#include <driver/i2c.h>
#include <mutex>

class I2C{
public:
    I2C(i2c_port_t port, int sda, int scl);
    esp_err_t read_reg(uint8_t addr, uint8_t reg, uint8_t *out, size_t bytes);
    esp_err_t write_reg(uint8_t addr, uint8_t reg, uint8_t *in, size_t bytes);
//    ~I2C();
private:
    std::mutex i2c_lock;
    i2c_port_t _port;

};