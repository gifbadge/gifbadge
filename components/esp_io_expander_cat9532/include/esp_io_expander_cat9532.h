#pragma once

#include <stdint.h>

#include "driver/i2c.h"
#include "esp_err.h"

#include "esp_io_expander.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Create a new CAT9532 IO expander driver
 *
 * @note The I2C communication should be initialized before use this function
 *
 * @param i2c_num: I2C port num
 * @param i2c_address: I2C address of chip
 * @param handle: IO expander handle
 *
 * @return
 *      - ESP_OK: Success, otherwise returns ESP_ERR_xxx
 */
esp_err_t esp_io_expander_new_i2c_cat9532(i2c_port_t i2c_num, uint32_t i2c_address, esp_io_expander_handle_t *handle);

#ifdef __cplusplus
}
#endif