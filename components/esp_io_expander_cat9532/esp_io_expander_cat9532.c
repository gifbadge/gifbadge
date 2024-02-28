/*
 * SPDX-FileCopyrightText: 2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <inttypes.h>
#include <string.h>
#include <stdlib.h>

#include "driver/i2c.h"
#include "esp_bit_defs.h"
#include "esp_check.h"
#include "esp_log.h"

#include "esp_io_expander.h"
#include "esp_io_expander_cat9532.h"

/* Timeout of each I2C communication */
#define I2C_TIMEOUT_MS          (10)

#define IO_COUNT                (16)

/* Default register value on power-up */
#define DIR_REG_DEFAULT_VAL     (0xff)
#define OUT_REG_DEFAULT_VAL     (0x00)

#define cat9532_reg_INPUT0  (0x00)
#define cat9532_reg_INPUT1  (0x01)
#define cat9532_reg_PSC0 (0x02)
#define cat9532_reg_PWM0  (0x03)
#define cat9532_reg_PSC1  (0x04)
#define cat9532_reg_PWM1  (0x05)
#define cat9532_reg_LS0  (0x06)
#define cat9532_reg_LS1  (0x07)
#define cat9532_reg_LS2  (0x08)
#define cat9532_reg_LS3  (0x09)

/**
 * @brief Device Structure Type
 *
 */
typedef struct {
  esp_io_expander_t base;
  i2c_port_t i2c_num;
  uint32_t i2c_address;
  struct {
    uint16_t direction;
    uint16_t output;
  } regs;
} esp_io_expander_cat9532_t;

static char *TAG = "cat9532";

static esp_err_t read_input_reg(esp_io_expander_handle_t handle, uint32_t *value);
static esp_err_t write_output_reg(esp_io_expander_handle_t handle, uint32_t value);
static esp_err_t read_output_reg(esp_io_expander_handle_t handle, uint32_t *value);
static esp_err_t write_direction_reg(esp_io_expander_handle_t handle, uint32_t value);
static esp_err_t read_direction_reg(esp_io_expander_handle_t handle, uint32_t *value);
static esp_err_t reset(esp_io_expander_t *handle);
static esp_err_t del(esp_io_expander_t *handle);

esp_err_t esp_io_expander_new_i2c_cat9532(i2c_port_t i2c_num, uint32_t i2c_address, esp_io_expander_handle_t *handle)
{
  ESP_RETURN_ON_FALSE(i2c_num < I2C_NUM_MAX, ESP_ERR_INVALID_ARG, TAG, "Invalid i2c num");
  ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG, "Invalid handle");

  esp_io_expander_cat9532_t *cat9532 = (esp_io_expander_cat9532_t *)calloc(1, sizeof(esp_io_expander_cat9532_t));
  ESP_RETURN_ON_FALSE(cat9532, ESP_ERR_NO_MEM, TAG, "Malloc failed");

  cat9532->base.config.io_count = IO_COUNT;
  cat9532->base.config.flags.dir_out_bit_zero = 1;
  cat9532->base.config.flags.output_high_bit_zero = 1;
  cat9532->i2c_num = i2c_num;
  cat9532->i2c_address = i2c_address;
  cat9532->base.read_input_reg = read_input_reg;
  cat9532->base.write_output_reg = write_output_reg;
  cat9532->base.read_output_reg = read_output_reg;
  cat9532->base.write_direction_reg = write_direction_reg;
  cat9532->base.read_direction_reg = read_direction_reg;
  cat9532->base.del = del;
  cat9532->base.reset = reset;
  cat9532->regs.output = 0x00;

  esp_err_t ret = ESP_OK;
  /* Reset configuration and register status */
  ESP_GOTO_ON_ERROR(reset(&cat9532->base), err, TAG, "Reset failed");

  *handle = &cat9532->base;
  return ESP_OK;
  err:
  free(cat9532);
  return ret;
}

static esp_err_t read_input_reg(esp_io_expander_handle_t handle, uint32_t *value)
{
  esp_io_expander_cat9532_t *cat9532 = (esp_io_expander_cat9532_t *)__containerof(handle, esp_io_expander_cat9532_t, base);

  uint8_t temp[2] = {0, 0};
  ESP_RETURN_ON_ERROR(
      i2c_master_write_read_device(cat9532->i2c_num, cat9532->i2c_address, (uint8_t[]){cat9532_reg_INPUT0|(1<<4)}, 1, (uint8_t*)&temp, 2, pdMS_TO_TICKS(I2C_TIMEOUT_MS)),
      TAG, "Read input reg failed");

  *value = (((uint32_t)temp[1]) << 8) | (temp[0]);
  return ESP_OK;
}

static esp_err_t write_output_reg(esp_io_expander_handle_t handle, uint32_t value)
{
  esp_io_expander_cat9532_t *cat9532 = (esp_io_expander_cat9532_t *)__containerof(handle, esp_io_expander_cat9532_t, base);

  value &= 0xffff;

  uint32_t out = 0;

  for (int x = 15; x >= 0; x--) {
    out |= ((value>>x)&0x01) << (x)*2;
  }

//  ESP_LOGI(TAG, "0x%lx 0x%lx", value, out);

  uint8_t data[] = {cat9532_reg_LS0|(1<<4), out&0xff, (out>>8)&0xff, (out>>16)&0xff, (out>>24)&0xff };
//  ESP_LOGI(TAG, "OUT_REG 0x%x 0x%x 0x%x 0x%x 0x%x ", data[0], data[1], data[2], data[3], data[4]);
  ESP_RETURN_ON_ERROR(
      i2c_master_write_to_device(cat9532->i2c_num, cat9532->i2c_address, data, sizeof(data), pdMS_TO_TICKS(I2C_TIMEOUT_MS)),
      TAG, "Write output reg failed");
  cat9532->regs.output = value;
  return ESP_OK;
}

static esp_err_t read_output_reg(esp_io_expander_handle_t handle, uint32_t *value)
{
  esp_io_expander_cat9532_t *cat9532 = (esp_io_expander_cat9532_t *)__containerof(handle, esp_io_expander_cat9532_t, base);

  *value = cat9532->regs.output;
  return ESP_OK;
}

static esp_err_t write_direction_reg(esp_io_expander_handle_t handle, uint32_t value)
{
  esp_io_expander_cat9532_t *cat9532 = (esp_io_expander_cat9532_t *)__containerof(handle, esp_io_expander_cat9532_t, base);
  value &= 0xff;
  cat9532->regs.direction = value;
  return ESP_OK;
}

static esp_err_t read_direction_reg(esp_io_expander_handle_t handle, uint32_t *value)
{
  esp_io_expander_cat9532_t *cat9532 = (esp_io_expander_cat9532_t *)__containerof(handle, esp_io_expander_cat9532_t, base);

  *value = cat9532->regs.direction;
  return ESP_OK;
}

static esp_err_t reset(esp_io_expander_t *handle)
{
  ESP_RETURN_ON_ERROR(write_output_reg(handle, OUT_REG_DEFAULT_VAL), TAG, "Write output reg failed");
  return ESP_OK;
}

static esp_err_t del(esp_io_expander_t *handle)
{
  esp_io_expander_cat9532_t *cat9532 = (esp_io_expander_cat9532_t *)__containerof(handle, esp_io_expander_cat9532_t, base);

  free(cat9532);
  return ESP_OK;
}