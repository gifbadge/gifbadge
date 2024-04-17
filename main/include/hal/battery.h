#pragma once

struct BatteryStatus {
  double voltage;
  int soc;
  double rate;
};

class Battery {
 public:
  Battery() = default;

  virtual ~Battery() = default;

  virtual BatteryStatus read() = 0;

  virtual int pollInterval() = 0;

  virtual double getVoltage() = 0;

  virtual int getSoc() = 0;

  virtual double getRate() = 0;

  virtual bool isCharging() = 0;
};