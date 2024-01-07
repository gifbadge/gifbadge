#pragma once

#include <hal/adc_types.h>
#include <esp_adc/adc_oneshot.h>

#include "hal/battery.h"

class battery_analog final: public Battery{
public:
    explicit battery_analog(adc_channel_t);
    ~battery_analog() final;

    BatteryStatus read() final;

    int pollInterval() override { return 250;};

    double getVoltage() override {return 0;};

    int getSoc() override {return 0;};

    double getRate() override {return 0;};

//    if(battery_config->getSoc() > 4){
//        lv_style_set_text_color(&battery_style, lv_color_black());
//        lv_label_set_text(widget, LV_SYMBOL_BATTERY_FULL);
//    }
//    else if(battery_config->getVoltage() > 3.7){
//        lv_style_set_text_color(&battery_style, lv_color_black());
//        lv_label_set_text(widget, LV_SYMBOL_BATTERY_3);
//    }
//    else if(battery_config->getVoltage() > 3.6){
//        lv_style_set_text_color(&battery_style, lv_color_black());
//        lv_label_set_text(widget, LV_SYMBOL_BATTERY_2);
//
//    }
//    else if(battery_config->getVoltage() > 3.5){
//        lv_style_set_text_color(&battery_style, lv_color_black());
//        lv_label_set_text(widget, LV_SYMBOL_BATTERY_1);
//
//    }
//    else{
//        lv_style_set_text_color(&battery_style, lv_color_hex(0xFF0000));
//        lv_label_set_text(widget, LV_SYMBOL_BATTERY_EMPTY);
//
//    }

private:
    adc_oneshot_unit_handle_t adc_handle;
    adc_cali_handle_t calibration_scheme;
    double present_value = 0;
    double alpha = 0.05;
};