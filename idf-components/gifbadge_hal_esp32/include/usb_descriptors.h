/*******************************************************************************
 * Copyright (c) 2026 GifBadge
 *
 * SPDX-License-Identifier:   GPL-3.0-or-later
 ******************************************************************************/

#pragma once

#define _PID_MAP(itf, n) ((CFG_TUD_##itf) << (n))
#define USB_TUSB_PID (0x4000 | _PID_MAP(CDC, 0) | _PID_MAP(MSC, 1) | _PID_MAP(HID, 2) | \
_PID_MAP(MIDI, 3) | _PID_MAP(AUDIO, 4) | _PID_MAP(VENDOR, 5) )

//------------- Device Descriptor -------------//
const tusb_desc_device_t descriptor_dev = {
  .bLength = sizeof(descriptor_dev),
  .bDescriptorType = TUSB_DESC_DEVICE,
  .bcdUSB = 0x0200,

#if CFG_TUD_CDC
  // Use Interface Association Descriptor (IAD) for CDC
  // As required by USB Specs IAD's subclass must be common class (2) and protocol must be IAD (1)
  .bDeviceClass = TUSB_CLASS_MISC,
  .bDeviceSubClass = MISC_SUBCLASS_COMMON,
  .bDeviceProtocol = MISC_PROTOCOL_IAD,
#else
  .bDeviceClass = 0x00,
  .bDeviceSubClass = 0x00,
  .bDeviceProtocol = 0x00,
#endif

  .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,

#if CONFIG_TINYUSB_DESC_USE_ESPRESSIF_VID
  .idVendor = TINYUSB_ESPRESSIF_VID,
#else
  .idVendor = CONFIG_TINYUSB_DESC_CUSTOM_VID,
#endif

#if CONFIG_TINYUSB_DESC_USE_DEFAULT_PID
  .idProduct = USB_TUSB_PID,
#else
  .idProduct = CONFIG_TINYUSB_DESC_CUSTOM_PID,
#endif

  .bcdDevice = CONFIG_TINYUSB_DESC_BCD_DEVICE,

  .iManufacturer = 0x01,
  .iProduct = 0x02,
  .iSerialNumber = 0x03,

  .bNumConfigurations = 0x01
};



//------------- Interfaces enumeration -------------//
enum {
#if CFG_TUD_CDC
  ITF_NUM_CDC = 0,
  ITF_NUM_CDC_DATA,
#endif

#if CFG_TUD_CDC > 1
  ITF_NUM_CDC1,
  ITF_NUM_CDC1_DATA,
#endif

#if CFG_TUD_MSC
  ITF_NUM_MSC,
#endif

  ITF_NUM_DFU_MODE,

  ITF_NUM_TOTAL
};

enum {
  TUSB_DESC_TOTAL_LEN = TUD_CONFIG_DESC_LEN +
                        CFG_TUD_CDC * TUD_CDC_DESC_LEN +
                        CFG_TUD_MSC * TUD_MSC_DESC_LEN +
                        CFG_TUD_NCM * TUD_CDC_NCM_DESC_LEN +
                        CFG_TUD_VENDOR * TUD_VENDOR_DESC_LEN +
                        CFG_TUD_DFU * TUD_DFU_DESC_LEN(1)
};

enum {
  STRID_LANGID = 0,
  STRID_MANUFACTURER,
  STRID_PRODUCT,
  STRID_SERIAL,
#if CFG_TUD_CDC
  STRID_CDC_INTERFACE,
#endif

#if CFG_TUD_MSC
  STRID_MSC_INTERFACE,
#endif


  STRID_DFU_INTERFACE
};

//------------- USB Endpoint numbers -------------//
enum {
  // Available USB Endpoints: 5 IN/OUT EPs and 1 IN EP
  EP_EMPTY = 0,
#if CFG_TUD_CDC
  EPNUM_0_CDC_NOTIF,
  EPNUM_0_CDC,
#endif

#if CFG_TUD_CDC > 1
  EPNUM_1_CDC_NOTIF,
  EPNUM_1_CDC,
#endif

#if CFG_TUD_MSC
  EPNUM_MSC,
#endif

  EPNUM_DFU,
};

#define DFU_FUNC_ATTRS (DFU_ATTR_CAN_UPLOAD | DFU_ATTR_CAN_DOWNLOAD | DFU_ATTR_MANIFESTATION_TOLERANT)

//------------- Configuration Descriptor -------------//
constexpr uint8_t descriptor_fs_cfg[] = {
  // Configuration number, interface count, string index, total length, attribute, power in mA
  TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, TUSB_DESC_TOTAL_LEN, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),

#if CFG_TUD_CDC
  // Interface number, string index, EP notification address and size, EP data address (out, in) and size.
  TUD_CDC_DESCRIPTOR(ITF_NUM_CDC, STRID_CDC_INTERFACE, 0x80 | EPNUM_0_CDC_NOTIF, 8, EPNUM_0_CDC, 0x80 | EPNUM_0_CDC, 64),
#endif

#if CFG_TUD_CDC > 1
  // Interface number, string index, EP notification address and size, EP data address (out, in) and size.
  TUD_CDC_DESCRIPTOR(ITF_NUM_CDC1, STRID_CDC_INTERFACE, 0x80 | EPNUM_1_CDC_NOTIF, 8, EPNUM_1_CDC, 0x80 | EPNUM_1_CDC, 64),
#endif

#if CFG_TUD_MSC
  // Interface number, string index, EP Out & EP In address, EP size
  TUD_MSC_DESCRIPTOR(ITF_NUM_MSC, STRID_MSC_INTERFACE, EPNUM_MSC, 0x80 | EPNUM_MSC, 64),
#endif

  TUD_DFU_DESCRIPTOR(ITF_NUM_DFU_MODE, 1, STRID_DFU_INTERFACE, DFU_FUNC_ATTRS, 1000, CFG_TUD_DFU_XFER_BUFSIZE)
};