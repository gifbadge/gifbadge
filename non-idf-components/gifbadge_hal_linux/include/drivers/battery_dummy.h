#pragma once

#include <hal/battery.h>

class battery_dummy final : public Battery {
 public:
  battery_dummy();
  ~battery_dummy() final = default;

  void poll();

  double BatteryVoltage() override;

  int getSoc() override;

  void removed() override;

  void inserted() override;

  State status() override;
};