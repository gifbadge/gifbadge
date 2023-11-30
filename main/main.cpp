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
#include "usb.h"
#include "display.h"
#include "config.h"
#include "ft5x06.h"

static const char *TAG = "MAIN";

struct BatteryArgs {
    std::shared_ptr<BatteryConfig> battery_config;
};


void battery_task(void *params) {
    auto *args = (BatteryArgs *) params;

    adc_oneshot_unit_handle_t adc1_handle;
    adc_oneshot_unit_init_cfg_t init_config1 = {
            .unit_id = ADC_UNIT_1,
            .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));
    adc_oneshot_chan_cfg_t config = {
            .atten = ADC_ATTEN_DB_11,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_9, &config));
    ESP_LOGI(TAG, "calibration scheme version is %s", "Curve Fitting");
    adc_cali_handle_t calibration_scheme;
    adc_cali_curve_fitting_config_t cali_config = {
            .unit_id = ADC_UNIT_1,
            .atten = ADC_ATTEN_DB_11,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    ESP_ERROR_CHECK(adc_cali_create_scheme_curve_fitting(&cali_config, &calibration_scheme));
    int reading;
    int voltage;
    adc_oneshot_read(adc1_handle, ADC_CHANNEL_9, &reading);
    ESP_ERROR_CHECK(adc_cali_raw_to_voltage(calibration_scheme, reading, &voltage));
    double m_present_value = voltage / 1000.00;
    double m_alpha = 0.05;
    while (true) {
        adc_oneshot_read(adc1_handle, ADC_CHANNEL_9, &reading);
        ESP_ERROR_CHECK(adc_cali_raw_to_voltage(calibration_scheme, reading, &voltage));
        m_present_value = (voltage / 1000.00) * (m_alpha) + m_present_value * (1.0f - m_alpha);
//        ESP_LOGI(TAG, "Voltage %f %f", (voltage * 2) / 1000.00, (m_present_value * 2));
        args->battery_config->setVoltage( (m_present_value * 2));
        vTaskDelay(250 / portTICK_PERIOD_MS);
    }
}

static void touchpad_init(){
    // Valid touching detect threshold
    i2c_master_write_to_device(I2C_NUM_0, 0x38, (const uint8_t []){FT5x06_ID_G_THGROUP, 70}, 2, 100 / portTICK_PERIOD_MS);


    // valid touching peak detect threshold
    i2c_master_write_to_device(I2C_NUM_0, 0x38, (const uint8_t []){FT5x06_ID_G_THPEAK, 60}, 2, 100 / portTICK_PERIOD_MS);

    // Touch focus threshold
    i2c_master_write_to_device(I2C_NUM_0, 0x38, (const uint8_t []){FT5x06_ID_G_THCAL, 16}, 2, 100 / portTICK_PERIOD_MS);

    // threshold when there is surface water
    i2c_master_write_to_device(I2C_NUM_0, 0x38, (const uint8_t []){FT5x06_ID_G_THWATER, 60}, 2, 100 / portTICK_PERIOD_MS);

    // threshold of temperature compensation
    i2c_master_write_to_device(I2C_NUM_0, 0x38, (const uint8_t []){FT5x06_ID_G_THTEMP, 10}, 2, 100 / portTICK_PERIOD_MS);

    // Touch difference threshold
    i2c_master_write_to_device(I2C_NUM_0, 0x38, (const uint8_t []){FT5x06_ID_G_THDIFF, 20}, 2, 100 / portTICK_PERIOD_MS);

    // Delay to enter 'Monitor' status (s)
    i2c_master_write_to_device(I2C_NUM_0, 0x38, (const uint8_t []){FT5x06_ID_G_TIME_ENTER_MONITOR, 2}, 2, 100 / portTICK_PERIOD_MS);

    // Period of 'Active' status (ms)
    i2c_master_write_to_device(I2C_NUM_0, 0x38, (const uint8_t []){FT5x06_ID_G_PERIODACTIVE, 12}, 2, 100 / portTICK_PERIOD_MS);

    // Timer to enter 'idle' when in 'Monitor' (ms)
    i2c_master_write_to_device(I2C_NUM_0, 0x38, (const uint8_t []){FT5x06_ID_G_PERIODMONITOR, 40}, 2, 100 / portTICK_PERIOD_MS);
}

struct i2c_args {
    QueueHandle_t touch_queue;
    QueueHandle_t accel_queue;
};

void i2c(void *arg){
    auto args = (i2c_args *) arg;
    QueueHandle_t touch_queue = args->touch_queue;

    i2c_config_t conf = {
            .mode = I2C_MODE_MASTER,
            .sda_io_num = 21,
            .scl_io_num = 18,
            .sda_pullup_en = GPIO_PULLUP_DISABLE,
            .scl_pullup_en = GPIO_PULLUP_DISABLE,
            .master = {.clk_speed = 400*1000},
            .clk_flags = 0,
    };
    i2c_param_config(I2C_NUM_0, &conf);
    i2c_driver_install(I2C_NUM_0, I2C_MODE_MASTER, 0, 0, 0);

    touchpad_init();




    while(true) {
//        ESP_LOGI(TAG, "I2C READ");
        uint8_t reg_addr = 0x0E;
        uint8_t data;
//        i2c_master_write_read_device(I2C_NUM_0, 0x15, &reg_addr, 1, &data, 1, 100 / portTICK_PERIOD_MS);
//        ESP_LOGI(TAG, "Got 0x%x", data);
//        reg_addr = 0x01;
//        data = 0x00;
//        i2c_master_write_read_device(I2C_NUM_0, 0x38, &reg_addr, 1, &data, 1, 100 / portTICK_PERIOD_MS);
//        ESP_LOGI(TAG, "Got 0x%x", data);


        //Touch Read
        i2c_master_write_read_device(I2C_NUM_0, 0x38, (const uint8_t []){FT5x06_TOUCH_POINTS}, 1, &data, sizeof(data), 100 / portTICK_PERIOD_MS);
        if((data & 0x0f)> 0 && (data & 0x0f) < 5) {
            uint8_t tmp[4] = {0};
            i2c_master_write_read_device(I2C_NUM_0, 0x38, (const uint8_t[]) {FT5x06_TOUCH1_XH}, 1, tmp, sizeof(tmp),
                                         100 / portTICK_PERIOD_MS);
//            uint16_t x = ((tmp[0] & 0x0f) << 8) + tmp[1]; //not rotated
//            uint16_t y = ((tmp[2] & 0x0f) << 8) + tmp[3];
            uint16_t y = ((tmp[0] & 0x0f) << 8) + tmp[1]; // rotated
            uint16_t x = ((tmp[2] & 0x0f) << 8) + tmp[3];
            y = 480-y;
            if(x > 0 || y > 0) {
                touch_event event = {esp_timer_get_time(), 0, x, y};
                xQueueSendToBack(touch_queue, &event, 100);

            }
        }

        vTaskDelay(50/portTICK_PERIOD_MS);
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
    MAIN_LOW_BATT
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
    xTaskCreate(input_task, "input_task", 10000, input_queue, 2, nullptr);

    TaskHandle_t display_task_handle = nullptr;

    display_task_args args = {
            nullptr,
            nullptr,
            imageconfig,
    };
    lcd_init(&args.panel_handle, &args.io_handle);

    xTaskCreate(display_task, "display_task", 20000, &args, 2, &display_task_handle);

    auto batteryconfig = std::make_shared<BatteryConfig>();
    BatteryArgs batteryargs = {batteryconfig};
    xTaskCreate(battery_task, "battery_task", 20000, &batteryargs, 2, nullptr);

    sharedState configState {
        imageconfig,
        batteryconfig
    };

    xTaskCreate(dump_state, "dump_state", 10000, &configState, 2, nullptr);


    QueueHandle_t touch_queue = xQueueCreate(20, sizeof(touch_event));
    i2c_args i2CArgs = {touch_queue};
    xTaskCreate(i2c, "i2c_task", 10000, &i2CArgs, 2, nullptr);


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
//                    vTaskDelay(500 / portTICK_PERIOD_MS);
                    xTaskNotifyIndexed(display_task_handle, 0, DISPLAY_FILE, eSetValueWithOverwrite );
                    break;
                case MAIN_USB:
                    xTaskNotifyIndexed(display_task_handle, 0, DISPLAY_USB, eSetValueWithOverwrite );
                    break;
                case MAIN_LOW_BATT:
                    xTaskNotifyIndexed(display_task_handle, 0, DISPLAY_BATT, eSetValueWithOverwrite );
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


        vTaskDelay(100 / portTICK_PERIOD_MS);
    }


}
