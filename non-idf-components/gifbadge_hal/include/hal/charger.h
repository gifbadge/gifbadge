#pragma once
#include <cstdint>

class Charger {
 public:

  /***
   * Enables the charger
   */
  virtual void ChargeEnable() = 0;

  /***
   * Disables the charger
   */
  virtual void ChargeDisable() = 0;

  /***
   * Reports if battery is detected
   * @return true if battery detected, otherwise false
   */
  virtual bool ChargeBattDetect() = 0;

  /***
   * Gets the currently set charge current limit in milliAmps
   * @return current in milliAmps
   */
  virtual void ChargeCurrentSet(uint16_t iset) = 0;

  /***
   * Gets the currently set charge current limit in milliAmps
   * @return current in milliAmps
   */
  virtual uint16_t ChargeCurrentGet() = 0;

 /***
 * Gets the currently set discharge current limit in milliAmps
 * @return current in milliAmps
 */
  virtual void DischargeCurrentSet(uint16_t iset) = 0;

 /***
 * Gets the currently set discharge current limit in milliAmps
 * @return current in milliAmps
 */
  virtual uint16_t DischargeCurrentGet() = 0;

  /***
   * Sets the charge termination voltage in milliVolts
   * @param vterm charge termination in millivolts
   */
  virtual void ChargeVtermSet(uint16_t vterm) = 0;

 /***
 * Gets the charge termination voltage in milliVolts
 * @return voltage in milliVolts
 */
  virtual uint16_t ChargeVtermGet() = 0;

  enum class ChargeStatus {
    NONE,
    BATTERYDETECTED,
    COMPLETED,
    TRICKLECHARGE,
    CONSTANTCURRENT,
    CONSTANTVOLTAGE,
    RECHARGE,
    PAUSED,
    SUPPLEMENTACTIVE,
    ERROR
  };

  /***
   * Gets the current status of the charger
   * @return charger status
   */
  virtual ChargeStatus ChargeStatusGet() = 0;

  enum class ChargeError {
    NONE,
    NTCSENSORERROR,
    VBATSENSORERROR,
    VBATLOW,
    VTRICKLE,
    MEASTIMEOUT,
    CHARGETIMEOUT,
    TRICKLETIMEOUT,
  };

 /***
 * Gets the current error of the charger if any
 * @return charger error
 */
  virtual ChargeError ChargeErrorGet() = 0;

};