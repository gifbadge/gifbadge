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
#include "esp_adc/adc_cali_scheme.h"
#include "esp_wifi.h"


#include "esp_vfs.h"

#include "menu.h"
#include "input.h"
#include "usb.h"
#include "display.h"
#include "config.h"


static const char *TAG = "MAIN";


void battery_task([[maybe_unused]] void *params) {
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
    float m_present_value = 3.3 / 2;
    float m_alpha = 0.05;
    while (true) {
        int reading;
        int voltage;
        adc_oneshot_read(adc1_handle, ADC_CHANNEL_9, &reading);
        ESP_ERROR_CHECK(adc_cali_raw_to_voltage(calibration_scheme, reading, &voltage));
        m_present_value = (voltage / 1000.00) * (m_alpha) + m_present_value * (1.0f - m_alpha);
        ESP_LOGI(TAG, "Voltage %f %f", (voltage * 2) / 1000.00, (m_present_value * 2));
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
}

//void advance(void *params) {
//    size_t pos = 0;
//    while (true) {
//        if (!gpio_get_level(static_cast<gpio_num_t>(0))) {
//            printf("Button Pressed, %i\n", pos);
//            if (pos == files.size() - 1) {
//                pos = 0;
//            } else {
//                pos++;
//            }
//            display_queue_element f = {};
//            strcpy(f.path, files.at(pos).c_str());
//            xQueueSendToBackFromISR(display_queue, &f, 0);
//            vTaskDelay(1000 / portTICK_PERIOD_MS);
//        }
//        vTaskDelay(250 / portTICK_PERIOD_MS);
//    }
//}

void dump_locks(void *arg){
    while(true){
        esp_pm_dump_locks(stdout);
        vTaskDelay(500/portTICK_PERIOD_MS);
    }
}

extern "C" void app_main(void) {
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    storage_init();

//    esp_pm_config_t pm_config = {.max_freq_mhz = 80, .min_freq_mhz = 40, .light_sleep_enable = true};
//    esp_pm_configure(&pm_config);

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
//    xTaskCreate(dump_locks, "dump_locks", 10000, nullptr, 2, nullptr);


    bool usb_on = true;
    std::unique_ptr<nvs::NVSHandle> handle = nvs::open_nvs_handle("usb", NVS_READWRITE, &err);
    handle->get_item("usb_on", usb_on);
//    if(usb_on) {
//        if (storage_free()) {
//            xTaskNotifyIndexed(display_task_handle, 0, DISPLAY_FILE, eSetValueWithOverwrite );
//        }
//
//        storage_callback([](bool state) {
//            ESP_LOGI(TAG, "state %u", state);
//            TaskHandle_t handle = xTaskGetHandle("display_task");
//            if (state) {
//                xTaskNotifyIndexed(handle, 0, DISPLAY_FILE, eSetValueWithOverwrite );
//            } else {
//                xTaskNotifyIndexed(handle, 0, DISPLAY_USB, eSetValueWithOverwrite );
//            }
//        });
//    } else {
//        xTaskNotifyIndexed(display_task_handle, 0, DISPLAY_FILE, eSetValueWithOverwrite );
//    }
            xTaskNotifyIndexed(display_task_handle, 0, DISPLAY_FILE, eSetValueWithOverwrite );


//    xTaskCreate(battery_task, "battery_task", 20000, nullptr, 2, nullptr);


    Menu *menu = new Menu(args.panel_handle, imageconfig);

    input_event i;
//    while (true) {
//        if (xQueueReceive(input_queue, (void *) &i, 50 / portTICK_PERIOD_MS)) {
//            get_event(i);
//        }
//    }

//    INPUT_EVENTS i;
    while (true) {
        if (xQueuePeek(input_queue, (void *) &i, 50 / portTICK_PERIOD_MS) && !menu->is_open()) {
            get_event(i);
            if (i.code == KEY_ENTER && i.value == STATE_PRESSED) {
                xQueueReceive(input_queue, (void *) &i, 0);
                    menu->open(args.io_handle, input_queue);
                    xTaskNotifyIndexed(display_task_handle, 0, DISPLAY_MENU, eSetValueWithOverwrite );
            }
            if((esp_timer_get_time()-i.timestamp)/1000 > 200){
                xQueueReceive(input_queue, (void *) &i, 0);
            }
            vTaskDelay(100 / portTICK_PERIOD_MS);
        }
    }


}
