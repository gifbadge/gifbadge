/*
 * SPDX-FileCopyrightText: 2015-2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <inttypes.h>
#include <stdlib.h>

#include "esp_check.h"
#include "esp_log.h"

#include "esp_io_expander.h"
#include "drivers/esp_io_expander_gpio.h"
#include "hal/gpio.h"
#include "log.h"

#define IO_COUNT                (1)



/* Default register value on power-up */
#define DIR_REG_DEFAULT_VAL     (0xffff)
#define OUT_REG_DEFAULT_VAL     (0x0000)

/**
 * @brief Device Structure Type
 *
 */
typedef struct {
  esp_io_expander_t base;
  hal::gpio::Gpio *gpio;
  struct {
    uint16_t direction;
    uint16_t output;
  } regs;
} esp_io_expander_gpio_t;

static char *TAG = "esp_io_expander_gpio";

static esp_err_t read_input_reg(esp_io_expander_handle_t handle, uint32_t *value);
static esp_err_t write_output_reg(esp_io_expander_handle_t handle, uint32_t value);
static esp_err_t read_output_reg(esp_io_expander_handle_t handle, uint32_t *value);
static esp_err_t write_direction_reg(esp_io_expander_handle_t handle, uint32_t value);
static esp_err_t read_direction_reg(esp_io_expander_handle_t handle, uint32_t *value);
static esp_err_t reset(esp_io_expander_t *handle);
static esp_err_t del(esp_io_expander_t *handle);

esp_err_t esp_io_expander_new_gpio(hal::gpio::Gpio *gpio, esp_io_expander_handle_t *handle)
{
  ESP_RETURN_ON_FALSE(gpio != nullptr, ESP_ERR_INVALID_ARG, TAG, "Invalid gpio");
  ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG, "Invalid handle");

  auto *tca = (esp_io_expander_gpio_t *)calloc(1, sizeof(esp_io_expander_gpio_t));
  ESP_RETURN_ON_FALSE(tca, ESP_ERR_NO_MEM, TAG, "Malloc failed");

  tca->gpio = gpio;
  tca->base.config.io_count = IO_COUNT;
  tca->base.config.flags.dir_out_bit_zero = 1;
  tca->base.read_input_reg = read_input_reg;
  tca->base.write_output_reg = write_output_reg;
  tca->base.read_output_reg = read_output_reg;
  tca->base.write_direction_reg = write_direction_reg;
  tca->base.read_direction_reg = read_direction_reg;
  tca->base.del = del;
  tca->base.reset = reset;

  esp_err_t ret = ESP_OK;
  /* Reset configuration and register status */
  ESP_GOTO_ON_ERROR(reset(&tca->base), err, TAG, "Reset failed");

  *handle = &tca->base;
  return ESP_OK;
  err:
  free(tca);
  return ret;
}

static esp_err_t read_input_reg(esp_io_expander_handle_t handle, uint32_t *value)
{
  auto *tca = (esp_io_expander_gpio_t *)__containerof(handle, esp_io_expander_gpio_t, base);
  *value = tca->gpio->GpioRead();
  return ESP_OK;
}

static esp_err_t write_output_reg(esp_io_expander_handle_t handle, uint32_t value)
{
  auto *tca = (esp_io_expander_gpio_t *)__containerof(handle, esp_io_expander_gpio_t, base);
  tca->regs.output = value;
  tca->gpio->GpioWrite(value);
  return ESP_OK;
}

static esp_err_t read_output_reg(esp_io_expander_handle_t handle, uint32_t *value)
{
  esp_io_expander_gpio_t *tca = (esp_io_expander_gpio_t *)__containerof(handle, esp_io_expander_gpio_t, base);
  *value = tca->regs.output;
  return ESP_OK;
}

static esp_err_t write_direction_reg(esp_io_expander_handle_t handle, uint32_t value)
{
  auto *tca = (esp_io_expander_gpio_t *)__containerof(handle, esp_io_expander_gpio_t, base);
  tca->regs.direction = value;
  return ESP_OK;
}

static esp_err_t read_direction_reg(esp_io_expander_handle_t handle, uint32_t *value)
{
  auto *tca = (esp_io_expander_gpio_t *)__containerof(handle, esp_io_expander_gpio_t, base);
  *value = tca->regs.direction;
  return ESP_OK;
}

static esp_err_t reset(esp_io_expander_t *handle)
{
  return ESP_OK;
}

static esp_err_t del(esp_io_expander_t *handle)
{
  auto *tca = (esp_io_expander_gpio_t *)__containerof(handle, esp_io_expander_gpio_t, base);
  free(tca);
  return ESP_OK;
}
