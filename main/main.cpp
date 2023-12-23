#include <filesystem>
#include <vector>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_pm.h"

#include "esp_err.h"
#include "esp_log.h"

#include <cstring>
#include <esp_adc/adc_oneshot.h>
#include <nvs_flash.h>
#include <nvs_handle.hpp>
#include <esp_vfs_fat.h>
#include <hal/i2c_types.h>
#include <driver/i2c.h>
#include <driver/rtc_io.h>
#include <esp_sleep.h>
#include "esp_adc/adc_cali_scheme.h"


#include "menu.h"
#include "input.h"
#include "keys.h"
#include "usb.h"
#include "display.h"
#include "config.h"
#include "ft5x06.h"

#include "ota.h"
#include "battery.h"
#include "i2c.h"

static const char *TAG = "MAIN";


struct i2c_args {
    std::shared_ptr<I2C> bus;
    QueueHandle_t touch_queue;
    QueueHandle_t accel_queue;
};

void i2c(void *arg) {
    auto args = (i2c_args *) arg;

    while (true) {
        uint8_t data;
        args->bus->read_reg(0x15, 0x0E, &data, 1);
        ESP_LOGI(TAG, "Got 0x%x", data);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

}

struct sharedState {
    std::shared_ptr<ImageConfig> image_config;
    std::shared_ptr<BatteryConfig> battery_config;
};

void dump_state(void *arg){
    auto *args = (sharedState *) arg;

    while(true){
        if(false){
            esp_pm_dump_locks(stdout);
        }
        ESP_LOGI(TAG, "Voltage: %f", args->battery_config->getVoltage());
        vTaskDelay(1000/portTICK_PERIOD_MS);
    }
}
bool inputAllowed = true;

enum MAIN_STATES {
    MAIN_NONE,
    MAIN_NORMAL,
    MAIN_USB,
    MAIN_LOW_BATT,
    MAIN_OTA,
};

MAIN_STATES currentState = MAIN_NORMAL;

extern "C" void app_main(void) {
    esp_err_t err;


    err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    //    bool usb_on = true;
//    std::unique_ptr<nvs::NVSHandle> handle = nvs::open_nvs_handle("usb", NVS_READWRITE, &err);
//    handle->get_item("usb_on", usb_on);
//    if(usb_on) {
    storage_callback([](bool state) {
        ESP_LOGI(TAG, "state %u", state);
        if (state) {
            currentState = MAIN_NORMAL;
        } else {
            currentState = MAIN_USB;
        }
    });
//    }

    storage_init();

    std::unique_ptr<nvs::NVSHandle> handle = nvs::open_nvs_handle("display", NVS_READWRITE, &err);
    DISPLAY_TYPES display_type = DISPLAY_TYPE_NONE;
    err = handle->get_item("type", display_type);

    esp_pm_config_t pm_config = {.max_freq_mhz = 240, .min_freq_mhz = 40, .light_sleep_enable = true};
    if(display_type == 2) {
        //RGB LCD doesn't like some power savings
        pm_config = {.max_freq_mhz = 240, .min_freq_mhz = 80, .light_sleep_enable = false};
    }
    esp_pm_configure(&pm_config);

    auto imageconfig = std::make_shared<ImageConfig>();


    vTaskDelay(100 / portTICK_PERIOD_MS);

    QueueHandle_t input_queue = xQueueCreate(10, sizeof(input_event));
    input_init(input_queue);

    TaskHandle_t display_task_handle = nullptr;

    display_task_args args = {
            nullptr,
            nullptr,
            imageconfig,
    };
    lcd_init(&args.panel_handle, &args.io_handle);

    xTaskCreate(display_task, "display_task", 20000, &args, 2, &display_task_handle);

    auto batteryconfig = std::make_shared<BatteryConfig>();
    battery_analog_init(batteryconfig);

    sharedState configState {
        imageconfig,
        batteryconfig
    };

    xTaskCreate(dump_state, "dump_state", 10000, &configState, 2, nullptr);

    auto i2c_bus = std::make_shared<I2C>(I2C_NUM_0, 21, 18);
    QueueHandle_t touch_queue = xQueueCreate(20, sizeof(touch_event));
    i2c_args i2CArgs = {i2c_bus, touch_queue};
    xTaskCreate(i2c, "i2c_task", 10000, &i2CArgs, 2, nullptr);

    vTaskDelay(2000/portTICK_PERIOD_MS);
    ota_boot_info();
    ota_init();

    Menu *menu = new Menu(args.panel_handle, imageconfig, batteryconfig, display_type);

    input_event i;
    MAIN_STATES oldState = MAIN_NONE;
    int64_t last_change = esp_timer_get_time();
    while (true) {
        if (oldState != currentState) {
            ESP_LOGI(TAG, "State %d", currentState);
            switch (currentState) {
                case MAIN_NONE:
                    break;
                case MAIN_NORMAL:
                    if(oldState == MAIN_USB){
                        //Check for OTA File
                        if(ota_check()){
                            currentState = MAIN_OTA;
                            xTaskNotifyIndexed(display_task_handle, 0, DISPLAY_OTA, eSetValueWithOverwrite );
                        }
                    }
                    else {
//                    vTaskDelay(500 / portTICK_PERIOD_MS);
                        xTaskNotifyIndexed(display_task_handle, 0, DISPLAY_FILE, eSetValueWithOverwrite);
                    }
                    break;
                case MAIN_USB:
                    xTaskNotifyIndexed(display_task_handle, 0, DISPLAY_USB, eSetValueWithOverwrite );
                    break;
                case MAIN_LOW_BATT:
                    xTaskNotifyIndexed(display_task_handle, 0, DISPLAY_BATT, eSetValueWithOverwrite );
                    break;
                case MAIN_OTA:
                    break;
            }
            oldState = currentState;
        }
        else if (currentState == MAIN_NORMAL) {
            if (xQueuePeek(input_queue, (void *) &i, 50 / portTICK_PERIOD_MS) && !menu->is_open()) {
                get_event(i);
                if (i.code == KEY_ENTER && i.value == STATE_PRESSED) {
                    xQueueReceive(input_queue, (void *) &i, 0);
                    xTaskNotifyIndexed(display_task_handle, 0, DISPLAY_MENU, eSetValueWithOverwrite);
                    vTaskDelay(100/portTICK_PERIOD_MS);
                    menu->open(args.io_handle, input_queue, touch_queue);
                }
                if (i.code == KEY_UP && i.value == STATE_PRESSED) {
                    xQueueReceive(input_queue, (void *) &i, 0);
                    xTaskNotifyIndexed(display_task_handle, 0, DISPLAY_PREVIOUS, eSetValueWithOverwrite);
                }
                if (i.code == KEY_DOWN && i.value == STATE_PRESSED) {
                    xQueueReceive(input_queue, (void *) &i, 0);
                    xTaskNotifyIndexed(display_task_handle, 0, DISPLAY_NEXT, eSetValueWithOverwrite);
                }
                if ((esp_timer_get_time() - i.timestamp) / 1000 > 200) {
                    xQueueReceive(input_queue, (void *) &i, 0);
                }
            }
            if(menu->is_open()){
                //Eat input events when we can't use them
                xQueueReset(touch_queue);
                xQueueReset(input_queue);
            }
            touch_event e = {};
            if(xQueueReceive(touch_queue, (void *) &e, 0) && !menu->is_open()){
                if(((esp_timer_get_time()/1000) - (last_change/1000)) > 1000) {
                    last_change = esp_timer_get_time();
                    if (e.y < 50) {
                        xTaskNotifyIndexed(display_task_handle, 0, DISPLAY_MENU, eSetValueWithOverwrite);
                        vTaskDelay(100 / portTICK_PERIOD_MS);
                        xQueueReset(touch_queue);
                        menu->open(args.io_handle, input_queue, touch_queue);
                    }
                    if (e.x < 50) {
                        xTaskNotifyIndexed(display_task_handle, 0, DISPLAY_PREVIOUS, eSetValueWithOverwrite);
                        xQueueReset(touch_queue); //Clear the queue
                    }
                    if (e.x > 430) {
                        xTaskNotifyIndexed(display_task_handle, 0, DISPLAY_NEXT, eSetValueWithOverwrite);
                        xQueueReset(touch_queue);
                    }
                }
                ESP_LOGI(TAG, "x: %d y: %d", e.x, e.y);
            }
        }
        else {
            //Eat input events when we can't use them
            xQueueReset(touch_queue);
            xQueueReset(input_queue);
        }
        if(currentState != MAIN_USB) {
            if (batteryconfig->getVoltage() < 3.2) {
                if (batteryconfig->getVoltage() < 3.1) {
                    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);
                    rtc_gpio_pullup_en(static_cast<gpio_num_t>(0));
                    rtc_gpio_pulldown_dis(static_cast<gpio_num_t>(0));
                    esp_sleep_enable_ext0_wakeup(static_cast<gpio_num_t>(0), 0);
                    esp_deep_sleep_start();
                } else {
                    inputAllowed = false;
                    TaskHandle_t lvglHandle = xTaskGetHandle("LVGL");
                    xTaskNotifyIndexed(lvglHandle, 0, LVGL_STOP, eSetValueWithOverwrite);
                    currentState = MAIN_LOW_BATT;
                }
            } else if (batteryconfig->getVoltage() > 3.4 && currentState == MAIN_LOW_BATT) {
                currentState = MAIN_NORMAL;
            }
        }
        if(currentState == MAIN_OTA){
            ota_install();
        }


        vTaskDelay(100 / portTICK_PERIOD_MS);
    }


}
