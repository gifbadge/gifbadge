/*
 * SPDX-FileCopyrightText: 2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <stdint.h>

#include "driver/i2c.h"
#include "esp_err.h"

#include "esp_io_expander.h"
#include "hal/gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Create a new TCA95xx_16bit IO expander driver
 *
 * @note The I2C communication should be initialized before use this function
 *
 * @param i2c_num: I2C port num
 * @param i2c_address: I2C address of chip (\see esp_io_expander_tca_95xx_16bit_address)
 * @param handle: IO expander handle
 *
 * @return
 *      - ESP_OK: Success, otherwise returns ESP_ERR_xxx
 */
esp_err_t esp_io_expander_new_gpio(Gpio *gpio, esp_io_expander_handle_t *handle);



#ifdef __cplusplus
}
#endif
