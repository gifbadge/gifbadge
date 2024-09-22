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

class Board {
 public:
  Board() = default;
  virtual ~Board() = default;

  virtual Battery * getBattery() = 0;
  virtual Touch * getTouch() = 0;
  virtual Keys * getKeys() = 0;
  virtual Display * getDisplay() = 0;
  virtual Backlight * getBacklight() = 0;
  virtual Vbus *getVbus() { return nullptr;};
  virtual Charger *getCharger() {return nullptr;};
  virtual void powerOff() = 0;
  virtual void reset() = 0;
  virtual BOARD_POWER powerState() = 0;
  virtual void pmLock() = 0;
  virtual void pmRelease() = 0;
  virtual bool storageReady() = 0;
  virtual StorageInfo storageInfo() = 0;
  virtual int StorageFormat() = 0;
  virtual const char * name() = 0;
  virtual void *turboBuffer() = 0;
  virtual Config *getConfig() = 0;
  virtual void debugInfo() = 0;
  virtual bool usbConnected() = 0;
  virtual int usbCallBack(tusb_msc_callback_t callback) = 0;
  virtual const char *swVersion() = 0;
  virtual char *serialNumber() = 0;
  virtual void lateInit() = 0;

  enum class WAKEUP_SOURCE {
    NONE,
    VBUS,
    KEY,
    REBOOT,
  };

  virtual WAKEUP_SOURCE bootReason() {return WAKEUP_SOURCE::KEY;};

};
}


