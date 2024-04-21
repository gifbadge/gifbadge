#pragma once

struct BatteryStatus {
  double voltage;
  int soc;
  double rate;
};

class Battery {
 public:

  enum class State {
    OK = 0,
    CHARGING,
    CONNECTED_NOT_CHARGING,
    DISCHARGING,
    NOT_PRESENT,
    ERROR
  };


  Battery() = default;

  virtual ~Battery() = default;

  virtual BatteryStatus read() = 0;

  virtual int pollInterval() = 0;

  virtual double getVoltage() = 0;

  virtual int getSoc() = 0;

  virtual double getRate() = 0;

  virtual bool isCharging() = 0;

  virtual void removed() = 0;

  virtual void inserted() = 0;

  virtual State status() = 0;
};