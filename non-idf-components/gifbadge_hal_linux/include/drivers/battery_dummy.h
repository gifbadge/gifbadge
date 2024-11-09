#pragma once

#include <hal/battery.h>

class battery_dummy final : public Battery {
 public:
  battery_dummy();
  ~battery_dummy() final = default;

  void poll();

  double BatteryVoltage() override;

  int BatterySoc() override;

  void BatteryRemoved() override;

  void BatteryInserted() override;

  State BatteryStatus() override;
};