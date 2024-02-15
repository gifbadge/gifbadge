#include "hal/hal_usb.h"

/*
 * SPDX-FileCopyrightText: 2022-2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

/* DESCRIPTION:
 * This example contains code to make ESP32-S3 based device recognizable by USB-hosts as a USB Mass Storage Device.
 * It either allows the embedded application i.e. example to access the partition or Host PC accesses the partition over USB MSC.
 * They can't be allowed to access the partition at the same time.
 * For different scenarios and behaviour, Refer to README of this example.
 */

#include <cerrno>
#include <dirent.h>
#include <memory>
#include <nvs_handle.hpp>
#include <esp_flash_spi_init.h>
#include <sys/stat.h>
#include <sdmmc_cmd.h>
#include "esp_console.h"
#include "esp_check.h"
#include "esp_partition.h"
#include "tinyusb.h"
#include "tusb_msc_storage.h"
#include "tusb_cdc_acm.h"
#include "tusb_console.h"


static const char *TAG = "USB";

#define BASE_PATH "/data" // base path to mount the partition

// mount the partition and show all the files in BASE_PATH
static void mount() {
    ESP_LOGI(TAG, "Mount storage...");
    ESP_ERROR_CHECK(tinyusb_msc_storage_mount(BASE_PATH));
}

mount_callback callback = nullptr;

void storage_callback(mount_callback _callback) {
    if (_callback != nullptr) {
        callback = _callback;
    }

}

// callback that is delivered when storage is mounted/unmounted by application.
static void storage_mount_changed_cb(tinyusb_msc_event_t *event) {
    ESP_LOGI(TAG, "Storage mounted to application: %s", event->mount_changed_data.is_mounted ? "Yes" : "No");
    if (callback != nullptr) {
        callback(event->mount_changed_data.is_mounted);
    }
    if(event->mount_changed_data.is_mounted){

        // List all the files in this directory
        ESP_LOGI(TAG, "\nls command output:");
        struct dirent *d;
        DIR *dh = opendir(BASE_PATH);
        if (!dh) {
            if (errno == ENOENT) {
                //If the directory is not found
                ESP_LOGE(TAG, "Directory doesn't exist %s", BASE_PATH);
            } else {
                //If the directory is not readable then throw error and exit
                ESP_LOGE(TAG, "Unable to read directory %s", BASE_PATH);
            }
            return;
        }
        //While the next entry is not readable we will print directory files
        while ((d = readdir(dh)) != nullptr) {
            FILINFO info;
//            char path[300];
//            snprintf(path, sizeof(path), "/data/%s", d->d_name);
            f_stat(d->d_name, &info);
            printf("%s %x\n", d->d_name, info.fattrib&AM_HID);
        }
    }
}

esp_err_t storage_init_spiflash(wl_handle_t *wl_handle) {
    ESP_LOGI(TAG, "Initializing wear levelling");

    const esp_partition_t *data_partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA,
                                                                     ESP_PARTITION_SUBTYPE_DATA_FAT, "data");
    if (data_partition == nullptr) {
        ESP_LOGE(TAG, "Failed to find FATFS partition. Check the partition table.");
        return ESP_ERR_NOT_FOUND;
    }

    return wl_mount(data_partition, wl_handle);
}

esp_err_t storage_init_ext_flash(int mosi, int miso, int sclk, int cs, wl_handle_t *wl_handle) {
    esp_err_t err;
    const spi_bus_config_t bus_config = {
            .mosi_io_num = mosi,
            .miso_io_num = miso,
            .sclk_io_num = sclk,
            .quadwp_io_num = -1,
            .quadhd_io_num = -1,
    };

    const esp_flash_spi_device_config_t device_config = {
            .host_id = SPI3_HOST,
            .cs_io_num = cs,
            .io_mode = SPI_FLASH_DIO,
            .cs_id = 0,
            .freq_mhz = 40,
    };

    ESP_LOGI(TAG, "Initializing external SPI Flash");
    ESP_LOGI(TAG, "Pin assignments:");
    ESP_LOGI(TAG, "MOSI: %2d   MISO: %2d   SCLK: %2d   CS: %2d",
             bus_config.mosi_io_num, bus_config.miso_io_num,
             bus_config.sclk_io_num, device_config.cs_io_num
    );
    // Initialize the SPI bus
    ESP_LOGI(TAG, "DMA CHANNEL: %d", SPI_DMA_CH_AUTO);
    ESP_ERROR_CHECK(spi_bus_initialize(SPI3_HOST, &bus_config, SPI_DMA_CH_AUTO));

    // Add device to the SPI bus
    esp_flash_t *ext_flash;
    ESP_ERROR_CHECK(spi_bus_add_flash_device(&ext_flash, &device_config));

    // Probe the Flash chip and initialize it
    err = esp_flash_init(ext_flash);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize external Flash: %s (0x%x)", esp_err_to_name(err), err);
    }

    // Print out the ID and size
    uint32_t id;
    ESP_ERROR_CHECK(esp_flash_read_id(ext_flash, &id));
    ESP_LOGI(TAG, "Initialized external Flash, size=%" PRIu32 " KB, ID=0x%" PRIx32, ext_flash->size / 1024, id);
    uint32_t size;
    esp_flash_get_physical_size(ext_flash, &size);
    ESP_LOGI(TAG, "Flash Size %" PRIu32 " KB", size);

    ESP_LOGI(TAG, "Adding external Flash as a partition, label=\"%s\", size=%" PRIu32 " KB", "ext_data",
             ext_flash->size / 1024);
    const esp_partition_t *fat_partition;
    const size_t offset = 0;
    ESP_ERROR_CHECK(
            esp_partition_register_external(ext_flash, offset, ext_flash->size, "ext_data", ESP_PARTITION_TYPE_DATA,
                                            ESP_PARTITION_SUBTYPE_DATA_FAT, &fat_partition));

    ESP_LOGI(TAG, "Listing data partitions:");
    esp_partition_iterator_t it = esp_partition_find(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, nullptr);

    for (; it != nullptr; it = esp_partition_next(it)) {
        const esp_partition_t *part = esp_partition_get(it);
        ESP_LOGI(TAG, "- partition '%s', subtype %d, offset 0x%" PRIx32 ", size %" PRIu32 " kB",
                 part->label, part->subtype, part->address, part->size / 1024);
    }

    esp_partition_iterator_release(it);

    ESP_LOGI(TAG, "Mounting FAT filesystem");
    const esp_vfs_fat_mount_config_t mount_config = {
            .format_if_mount_failed = true,
            .max_files = 4,
            .allocation_unit_size = CONFIG_WL_SECTOR_SIZE
    };
    err = esp_vfs_fat_spiflash_mount_rw_wl("/data", "ext_data", &mount_config, wl_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to mount FATFS (%s)", esp_err_to_name(err));
        return err;
    }
//    esp_vfs_fat_spiflash_unmount_rw_wl("/data", *wl_handle);
//    const esp_partition_t *data_partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA,
//                                                                     ESP_PARTITION_SUBTYPE_DATA_FAT, "ext_data");
//    if (data_partition == nullptr) {
//        ESP_LOGE(TAG, "Failed to find FATFS partition. Check the partition table.");
//        return ESP_ERR_NOT_FOUND;
//    }

//    return wl_mount(data_partition, wl_handle);
    return ESP_OK;
}

esp_err_t init_sdmmc_slot(gpio_num_t clk, gpio_num_t cmd, gpio_num_t d0, gpio_num_t d1, gpio_num_t d2, gpio_num_t d3,
                          gpio_num_t cd, sdmmc_card_t **card, int width)
{
    esp_err_t ret = ESP_OK;
    bool host_init = false;
    sdmmc_card_t *sd_card;

    ESP_LOGI(TAG, "Initializing SDCard");

    // By default, SD card frequency is initialized to SDMMC_FREQ_DEFAULT (20MHz)
    // For setting a specific frequency, use host.max_freq_khz (range 400kHz - 40MHz for SDMMC)
    // Example: for fixed frequency of 10MHz, use host.max_freq_khz = 10000;
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    host.max_freq_khz = SDMMC_FREQ_HIGHSPEED;

    // This initializes the slot without card detect (CD) and write protect (WP) signals.
    // Modify slot_config.gpio_cd and slot_config.gpio_wp if your board has these signals.
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();

    slot_config.width = width;

    slot_config.clk = clk;
    slot_config.cmd = cmd;
    slot_config.d0 = d0;
    slot_config.d1 = d1;
    slot_config.d2 = d2;
    slot_config.d3 = d3;
    slot_config.cd = cd;


//    // Enable internal pullups on enabled pins. The internal pullups
//    // are insufficient however, please make sure 10k external pullups are
//    // connected on the bus. This is for debug / example purpose only.
//    slot_config.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;

    // not using ff_memalloc here, as allocation in internal RAM is preferred
    sd_card = (sdmmc_card_t *)malloc(sizeof(sdmmc_card_t));
    ESP_GOTO_ON_FALSE(sd_card, ESP_ERR_NO_MEM, clean, TAG, "could not allocate new sdmmc_card_t");

    ESP_GOTO_ON_ERROR((*host.init)(), clean, TAG, "Host Config Init fail");
    host_init = true;

    ESP_GOTO_ON_ERROR(sdmmc_host_init_slot(host.slot, (const sdmmc_slot_config_t *) &slot_config),
                      clean, TAG, "Host init slot fail");

    while (sdmmc_card_init(&host, sd_card)) {
        ESP_LOGE(TAG, "The detection pin of the slot is disconnected(Insert uSD card). Retrying...");
        vTaskDelay(pdMS_TO_TICKS(3000));
    }

    // Card has been initialized, print its properties
    sdmmc_card_print_info(stdout, sd_card);
    *card = sd_card;

    return ESP_OK;

    clean:
    if (host_init) {
        if (host.flags & SDMMC_HOST_FLAG_DEINIT_ARG) {
            host.deinit_p(host.slot);
        } else {
            (*host.deinit)();
        }
    }
    if (sd_card) {
        free(sd_card);
        sd_card = nullptr;
    }
    return ret;
}

esp_err_t
mount_sdmmc_slot(gpio_num_t clk, gpio_num_t cmd, gpio_num_t d0, gpio_num_t d1, gpio_num_t d2, gpio_num_t d3,
                 gpio_num_t cd, sdmmc_card_t **card, int width)
{
    esp_err_t ret = ESP_OK;

    ESP_LOGI(TAG, "Initializing SDCard");

    // By default, SD card frequency is initialized to SDMMC_FREQ_DEFAULT (20MHz)
    // For setting a specific frequency, use host.max_freq_khz (range 400kHz - 40MHz for SDMMC)
    // Example: for fixed frequency of 10MHz, use host.max_freq_khz = 10000;
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    host.max_freq_khz = SDMMC_FREQ_HIGHSPEED;

    // This initializes the slot without card detect (CD) and write protect (WP) signals.
    // Modify slot_config.gpio_cd and slot_config.gpio_wp if your board has these signals.
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();

    slot_config.width = width;

    slot_config.clk = clk;
    slot_config.cmd = cmd;
    slot_config.d0 = d0;
    slot_config.d1 = d1;
    slot_config.d2 = d2;
    slot_config.d3 = d3;
    slot_config.cd = cd;

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
            .max_files = 5,
            .allocation_unit_size = 16 * 1024
    };
    ret = esp_vfs_fat_sdmmc_mount("/data", &host, &slot_config, &mount_config, card);
    sdmmc_card_print_info(stdout, *card);
    return ret;

}

bool storage_free() {
    return tinyusb_msc_storage_in_use_by_usb_host();
}



void storage_init_mmc(int usb_sense, sdmmc_card_t **card) {

    const tinyusb_msc_sdmmc_config_t config_sdmmc = {
            .card = *card,
            .callback_mount_changed = storage_mount_changed_cb,  /* First way to register the callback. This is while initializing the storage. */
            .mount_config = {.max_files = 5},
    };
    ESP_ERROR_CHECK(tinyusb_msc_storage_init_sdmmc(&config_sdmmc));
    ESP_ERROR_CHECK(tinyusb_msc_register_callback(TINYUSB_MSC_EVENT_MOUNT_CHANGED, storage_mount_changed_cb)); /* Other way to register the callback i.e. registering using separate API. If the callback had been already registered, it will be overwritten. */

    mount();

    const tinyusb_config_t tusb_cfg = {
            .device_descriptor = nullptr,
            .string_descriptor = nullptr,
            .string_descriptor_count = 0,
            .external_phy = false,
            .configuration_descriptor = nullptr,
            .self_powered = true,
            .vbus_monitor_io = usb_sense,
    };

    ESP_ERROR_CHECK(tinyusb_driver_install(&tusb_cfg));

}

void storage_init_wearlevel(int usb_sense, wl_handle_t *wl_handle ) {

    const tinyusb_msc_spiflash_config_t config_spi = {
            .wl_handle = *wl_handle,
            .callback_mount_changed = storage_mount_changed_cb  /* First way to register the callback. This is while initializing the storage. */
    };

        ESP_ERROR_CHECK(tinyusb_msc_storage_init_spiflash(&config_spi));
    ESP_ERROR_CHECK(tinyusb_msc_register_callback(TINYUSB_MSC_EVENT_MOUNT_CHANGED, storage_mount_changed_cb)); /* Other way to register the callback i.e. registering using separate API. If the callback had been already registered, it will be overwritten. */

    mount();

    const tinyusb_config_t tusb_cfg = {
            .device_descriptor = nullptr,
            .string_descriptor = nullptr,
            .string_descriptor_count = 0,
            .external_phy = false,
            .configuration_descriptor = nullptr,
            .self_powered = true,
            .vbus_monitor_io = usb_sense,
    };

    ESP_ERROR_CHECK(tinyusb_driver_install(&tusb_cfg));

}


void storage_init() {
    esp_err_t err;

    ESP_LOGI(TAG, "Initializing storage...");

    static wl_handle_t wl_handle = WL_INVALID_HANDLE;
//    ESP_ERROR_CHECK(storage_init_spiflash(&wl_handle));
    ESP_ERROR_CHECK(storage_init_ext_flash(39, 41, 40, 42, &wl_handle));


    const tinyusb_msc_spiflash_config_t config_spi = {
            .wl_handle = wl_handle,
            .callback_mount_changed = storage_mount_changed_cb  /* First way to register the callback. This is while initializing the storage. */
    };

    bool usb_on = true;
    std::unique_ptr<nvs::NVSHandle> handle = nvs::open_nvs_handle("usb", NVS_READWRITE, &err);
    if (err != ESP_OK) {
        printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
    } else {
        err = handle->get_item("usb_on", usb_on);
        switch (err) {
            case ESP_OK:
                ESP_LOGI(TAG, "USB MSC %s", usb_on ? "Enabled" : "Disabled");
                break;
            case ESP_ERR_NVS_NOT_FOUND:
                handle->set_item("usb_on", true);
                handle->commit();
                ESP_LOGI(TAG, "USB MSC %s", usb_on ? "Enabled" : "Disabled");
                break;
            default :
                printf("Error (%s) reading!\n", esp_err_to_name(err));
        }
    }
    if (true) {
//        // TODO: Make this work so USB can actually be turned off
        ESP_ERROR_CHECK(tinyusb_msc_storage_init_spiflash(&config_spi));
//        ESP_ERROR_CHECK(tinyusb_msc_register_callback(TINYUSB_MSC_EVENT_MOUNT_CHANGED,
//                                                      storage_mount_changed_cb)); /* Other way to register the callback i.e. registering using separate API. If the callback had been already registered, it will be overwritten. */
//        //mounted in the app by default
//        mount();
    }
    else {
        wl_unmount(wl_handle);
        ESP_LOGI(TAG, "Mounting FAT filesystem");
        const esp_vfs_fat_mount_config_t mount_config = {
                .format_if_mount_failed = true,
                .max_files = 4,
                .allocation_unit_size = CONFIG_WL_SECTOR_SIZE
        };
        err = esp_vfs_fat_spiflash_mount_rw_wl("/data", "ext_data", &mount_config, &wl_handle);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to mount FATFS (%s)", esp_err_to_name(err));
        }
    }




    ESP_LOGI(TAG, "USB initialization");
    const tinyusb_config_t tusb_cfg = {
            .device_descriptor = nullptr,
            .string_descriptor = nullptr,
            .string_descriptor_count = 0,
            .external_phy = false,
            .configuration_descriptor = nullptr,
    };
    ESP_ERROR_CHECK(tinyusb_driver_install(&tusb_cfg));

    ESP_LOGI(TAG, "USB MSC initialization DONE");
    tinyusb_config_cdcacm_t acm_cfg = {
            .usb_dev = TINYUSB_USBDEV_0,
            .cdc_port = TINYUSB_CDC_ACM_0,
            .callback_rx = nullptr, // the first way to register a callback
            .callback_rx_wanted_char = nullptr,
            .callback_line_state_changed = nullptr,
            .callback_line_coding_changed = nullptr
    };
    ESP_ERROR_CHECK(tusb_cdc_acm_init(&acm_cfg));
    esp_tusb_init_console(TINYUSB_CDC_ACM_0);
}

