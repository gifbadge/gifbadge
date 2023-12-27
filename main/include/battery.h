#pragma once
#include "config.h"
#include "hal/battery.h"

struct BatteryArgs {
    std::shared_ptr<BatteryConfig> battery_config;
    std::shared_ptr<Battery> battery;
};

void battery_init(const std::shared_ptr<Battery>&, const std::shared_ptr<BatteryConfig>& battery_config);