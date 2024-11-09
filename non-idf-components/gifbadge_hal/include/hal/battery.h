#pragma once
#include <cmath>

/**
 * Abstraction for battery hardware
 */
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

  /**
   * Get the battery voltage
   * @return voltage in V
   */
  virtual double BatteryVoltage() = 0;

  /**
   * Get the current being sourced or sunk by the battery
   * @return current in A
   */
  virtual double BatteryCurrent() { return NAN; }

  /**
   * Get the battery temperature
   * @return Temperature in °C
   */
  virtual double BatteryTemperature() { return NAN;}

  /**
   * Get the state of charge of the battery
   * @return charge in percent
   */
  virtual int getSoc() = 0;

  /**
   * Notify hardware that battery has been removed
   */
  virtual void removed() = 0;

  /**
   * Notify hardware that battery has been inserted
   */
  virtual void inserted() = 0;

  /**
   * Get the current status of the battery hardware
   * @return
   */
  virtual State status() = 0;
};