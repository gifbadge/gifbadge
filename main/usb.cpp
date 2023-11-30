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
#include "esp_console.h"
#include "esp_check.h"
#include "esp_partition.h"
#include "tinyusb.h"
#include "tusb_msc_storage.h"
#include "usb.h"
#include "tusb_cdc_acm.h"
#include "tusb_console.h"


static const char *TAG = "USB";

///* TinyUSB descriptors
//   ********************************************************************* */
//#define EPNUM_MSC       1
//#define TUSB_DESC_TOTAL_LEN (TUD_CONFIG_DESC_LEN + TUD_MSC_DESC_LEN)
//
//enum {
//    ITF_NUM_MSC = 0,
//    ITF_NUM_TOTAL
//};
//
//enum {
//    EDPT_CTRL_OUT = 0x00,
//    EDPT_CTRL_IN = 0x80,
//
//    EDPT_MSC_OUT = 0x01,
//    EDPT_MSC_IN = 0x81,
//};

//static uint8_t const desc_configuration[] = {
//        // Config number, interface count, string index, total length, attribute, power in mA
//        TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, TUSB_DESC_TOTAL_LEN, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),
//
//        // Interface number, string index, EP Out & EP In address, EP size
//        TUD_MSC_DESCRIPTOR(ITF_NUM_MSC, 0, EDPT_MSC_OUT, EDPT_MSC_IN, TUD_OPT_HIGH_SPEED ? 512 : 64),
//};

//static tusb_desc_device_t descriptor_config = {
//        .bLength = sizeof(descriptor_config),
//        .bDescriptorType = TUSB_DESC_DEVICE,
//        .bcdUSB = 0x0200,
//        .bDeviceClass = TUSB_CLASS_MISC,
//        .bDeviceSubClass = MISC_SUBCLASS_COMMON,
//        .bDeviceProtocol = MISC_PROTOCOL_IAD,
//        .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,
//        .idVendor = 0x303A, // This is Espressif VID. This needs to be changed according to Users / Customers
//        .idProduct = 0x4002,
//        .bcdDevice = 0x100,
//        .iManufacturer = 0x01,
//        .iProduct = 0x02,
//        .iSerialNumber = 0x03,
//        .bNumConfigurations = 0x01
//};
//
//static char const *string_desc_arr[] = {
//        (const char[]) {0x09, 0x04},  // 0: is supported language is English (0x0409)
//        "TinyUSB",                      // 1: Manufacturer
//        "TinyUSB Device",               // 2: Product
//        "123456",                       // 3: Serials
//        "Example MSC",                  // 4. MSC
//};
/*********************************************************************** TinyUSB descriptors*/

#define BASE_PATH "/data" // base path to mount the partition

// mount the partition and show all the files in BASE_PATH
static void mount() {
    ESP_LOGI(TAG, "Mount storage...");
    ESP_ERROR_CHECK(tinyusb_msc_storage_mount(BASE_PATH));

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
        printf("%s\n", d->d_name);
    }
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
}

static esp_err_t storage_init_spiflash(wl_handle_t *wl_handle) {
    ESP_LOGI(TAG, "Initializing wear levelling");

    const esp_partition_t *data_partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA,
                                                                     ESP_PARTITION_SUBTYPE_DATA_FAT, "data");
    if (data_partition == nullptr) {
        ESP_LOGE(TAG, "Failed to find FATFS partition. Check the partition table.");
        return ESP_ERR_NOT_FOUND;
    }

    return wl_mount(data_partition, wl_handle);
}

static esp_err_t storage_init_ext_flash(wl_handle_t *wl_handle) {
    esp_err_t err;
    const spi_bus_config_t bus_config = {
            .mosi_io_num = 39,
            .miso_io_num = 41,
            .sclk_io_num = 40,
            .quadwp_io_num = -1,
            .quadhd_io_num = -1,
    };

    const esp_flash_spi_device_config_t device_config = {
            .host_id = SPI3_HOST,
            .cs_io_num = 42,
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
    esp_vfs_fat_spiflash_unmount_rw_wl("/data", *wl_handle);
    const esp_partition_t *data_partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA,
                                                                     ESP_PARTITION_SUBTYPE_DATA_FAT, "ext_data");
    if (data_partition == nullptr) {
        ESP_LOGE(TAG, "Failed to find FATFS partition. Check the partition table.");
        return ESP_ERR_NOT_FOUND;
    }

    return wl_mount(data_partition, wl_handle);
}

bool storage_free() {
    return tinyusb_msc_storage_in_use_by_usb_host();
}

void storage_init() {
    esp_err_t err;

    ESP_LOGI(TAG, "Initializing storage...");

    static wl_handle_t wl_handle = WL_INVALID_HANDLE;
//    ESP_ERROR_CHECK(storage_init_spiflash(&wl_handle));
    ESP_ERROR_CHECK(storage_init_ext_flash(&wl_handle));


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
        // TODO: Make this work so USB can actually be turned off
        ESP_ERROR_CHECK(tinyusb_msc_storage_init_spiflash(&config_spi));
        ESP_ERROR_CHECK(tinyusb_msc_register_callback(TINYUSB_MSC_EVENT_MOUNT_CHANGED,
                                                      storage_mount_changed_cb)); /* Other way to register the callback i.e. registering using separate API. If the callback had been already registered, it will be overwritten. */
        //mounted in the app by default
        mount();
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
//                .device_descriptor = &descriptor_config,
//                .string_descriptor = string_desc_arr,
//                .string_descriptor_count = sizeof(string_desc_arr) / sizeof(string_desc_arr[0]),
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
