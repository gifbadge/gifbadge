#pragma once
#include <tinyusb.h>
#include <soc/gpio_num.h>

esp_err_t esp32s3_usb_init(gpio_num_t usb_sense);
