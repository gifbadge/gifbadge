#pragma once

#include <esp_err.h>
#include <hal/gpio_hal.h>
#include <driver/sdmmc_host.h>
#include <wear_levelling.h>

typedef void (*mount_callback)(bool);
void storage_callback(mount_callback _callback);


esp_err_t init_sdmmc_slot(gpio_num_t clk, gpio_num_t cmd, gpio_num_t d0, gpio_num_t d1, gpio_num_t d2, gpio_num_t d3,
                          gpio_num_t cd, sdmmc_card_t **card, int width);
esp_err_t
mount_sdmmc_slot(gpio_num_t clk, gpio_num_t cmd, gpio_num_t d0, gpio_num_t d1, gpio_num_t d2, gpio_num_t d3,
                 gpio_num_t cd, sdmmc_card_t **card, int width);
void storage_init_mmc(int usb_sense, sdmmc_card_t **card);
void storage_init_wearlevel(int usb_sense, wl_handle_t *wl_handle);
esp_err_t storage_init_spiflash(wl_handle_t *wl_handle);
esp_err_t storage_init_ext_flash(int mosi, int miso, int sclk, int cs, wl_handle_t *wl_handle);


