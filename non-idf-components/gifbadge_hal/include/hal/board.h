#pragma once

#include <memory>
#include "hal/battery.h"
#include "hal/touch.h"
#include "keys.h"
#include "display.h"
#include "hal/backlight.h"
#include "hal/storage.h"
//#include "ota.h"
#include "config.h"
#include "tusb_msc_storage.h"
#include "vbus.h"
#include "charger.h"

namespace Boards {

enum BOARD_POWER {
  BOARD_POWER_NORMAL,
  BOARD_POWER_LOW,
  BOARD_POWER_CRITICAL,
};

enum CHARGE_POWER{
  CHARGE_NONE,
  CHARGE_LOW,
  CHARGE_HIGH,
};

/*!
 * OTA Validation return value
 */
enum class OtaError {
  OK = 0,
  WRONG_CHIP,
  WRONG_BOARD,
  SAME_VERSION,
};


/**
 * The Board class represents the hardware gifbadge is running on.
 */
class Board {
 public:
  /**
   * The constructor should initialize the bare minimum of hardware.
   * For example the power management IC to enable charging, and find the reason it was powered on
   */
  Board() = default;
  virtual ~Board() = default;

  /**
   * Initialize the rest of the needed hardware
   */
  virtual void lateInit() = 0;

  /**
   * Get the instance of Battery used by the current board
   * @return Battery * or nullptr
   * @see Battery
   */
  virtual Battery * getBattery() = 0;

  /**
   * Get the instance of Touch used by the current board
   * @return Touch * or nullptr
   * @see Touch
   */
  virtual Touch * getTouch() = 0;

  /**
  * Get the instance of Keys used by the current board
  * @return Keys * or nullptr
   * @see Touch
  */
  virtual Keys * getKeys() = 0;

  /**
  * Get the instance of Display used by the current board
  * @return Display * or nullptr
   * @see Keys
  */
  virtual Display * getDisplay() = 0;

  /**
  * Get the instance of Backlight used by the current board
  * @return Backlight * or nullptr
   * @see Display
  */
  virtual Backlight * getBacklight() = 0;

  /**
  * Get the instance of Vbus used by the current board
  * @return Vbus * or nullptr
   * @see Vbus
  */
  virtual Vbus *getVbus() { return nullptr;};

  /**
  * Get the instance of Charger used by the current board
  * @return Charger * or nullptr
   * @see Charger
  */
  virtual Charger *getCharger() {return nullptr;};

  /**
  * Get the instance of Config used by the current board
  * @return Config * or nullptr
   * @see Config
  */
  virtual Config *getConfig() = 0;

  /**
   * poweroff if the hardware supports it. If not, deep sleep instead
   */
  virtual void powerOff() = 0;

  /**
   * reset the device
   */
  virtual void reset() = 0;

  /**
   * Get the power state to decide if a warning should be displayed or a power off should occur
   * @return
   */
  virtual BOARD_POWER powerState() = 0;

  /**
   * Acquire a power management lock to keep the device at maximum performance
   */
  virtual void pmLock() = 0;

  /**
   * Release the lock acquired by pmLock()
   */
  virtual void pmRelease() = 0;

  /**
   * Check if storage is present
   * @return true if storage is present otherwise false
   */
  virtual bool storageReady() = 0;

  /**
   * returns a StorageInfo structure containing information about the storage present in the device
   * @return
   */
  virtual StorageInfo storageInfo() = 0;

  /**
   * Formats the storage
   * @return 0 if successful, platform specific error otherwise
   */
  virtual int StorageFormat() = 0;

  /**
   * Gets the pointer to a region of memory allocated for GIFDecoding
   * This should be allocated in internal/fast memory
   * Size should be (Screen x Resolution * Screen y Resolution)+0x6100
   * @return pointer to the buffer or nullptr
   */
  virtual void *turboBuffer() = 0;

  /**
   * Print board specific information to stdout
   * Called periodically from a timer
   */
  virtual void debugInfo() = 0;

  /**
   * Check if connected to a USB Host
   * @return true if connected
   */
  virtual bool usbConnected() = 0;

  /**
   * Register a callback for USB Connect/Disconnect.
   * If the callback had been already registered, it will be overwritten
   * @param callback
   * @return 0 if successful
   */
  virtual int usbCallBack(tusb_msc_callback_t callback) = 0;

  /**
   * The name of current board
   * @return null terminated string
   */
  virtual const char * name() = 0;

  /**
   * The running software version
   * @return null terminated string
   */
  virtual const char *swVersion() = 0;

  /**
   * The device serial number
   * @return null terminated string
   */
  virtual char *serialNumber() = 0;


  // OTA Stuff
  /**
   * Print information to stdout.
   * E.G. Current boot partition
   */
  virtual void BootInfo() {};
  /**
   * Check if an OTA update image file exists on the storage media
   * @return true if one exists
   */
  virtual bool OtaCheck() { return false;};
  /**
   * Validates the OTA update file
   * @return OtaError
   */
  virtual OtaError OtaValidate() { return OtaError::OK; };
  /**
   * Start installing the OTA update
   */
  virtual void OtaInstall() {};
  /**
   * Get the current progress of the OTA update
   * @return 0 to 100%
   */
  virtual int OtaStatus() {return 100;};

  enum class WAKEUP_SOURCE {
    NONE,
    VBUS,
    KEY,
    REBOOT,
  };

  /**
   * The source of the wakeup. E.G. power button, hw reset
   * @return
   */
  virtual WAKEUP_SOURCE bootReason() {return WAKEUP_SOURCE::KEY;};

};
}


