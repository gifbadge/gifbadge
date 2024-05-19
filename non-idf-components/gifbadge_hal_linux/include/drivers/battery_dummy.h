#pragma once

#include <hal/battery.h>

class battery_dummy final : public Battery {
 public:
  battery_dummy();
  ~battery_dummy() final = default;

  void poll();

  int pollInterval() override { return 1000; };

  double getVoltage() override;

  int getSoc() override;

  double getRate() override;

  void removed() override;

  void inserted() override;

  State status() override;
};