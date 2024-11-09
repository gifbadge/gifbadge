#pragma once
#include <cmath>
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

  virtual int pollInterval() = 0;

  virtual double BatteryVoltage() = 0;

  virtual double BatteryCurrent() { return NAN; }

  virtual double BatteryTemperature() { return NAN;}

  virtual int getSoc() = 0;

  virtual void removed() = 0;

  virtual void inserted() = 0;

  virtual State status() = 0;
};