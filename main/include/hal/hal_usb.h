#pragma once

#include <esp_err.h>
#include <hal/gpio_hal.h>
#include <driver/sdmmc_host.h>

typedef void (*mount_callback)(bool);
void storage_callback(mount_callback _callback);


esp_err_t init_sdmmc_slot(gpio_num_t clk, gpio_num_t cmd, gpio_num_t d0, gpio_num_t cd, sdmmc_card_t **card);
void storage_init_mmc(int usb_sense, sdmmc_card_t **card);

