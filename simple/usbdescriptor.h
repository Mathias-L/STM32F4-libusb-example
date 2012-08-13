#ifndef USBDESCRIPTOR_H_INCLUDED
#define USBDESCRIPTOR_H_INCLUDED

#include "usb.h"

/*
 * This file contains the USB descriptors. For Details see
 *   http://www.beyondlogic.org/usbnutshell/usb5.shtml
 *   http://www.usbmadesimple.co.uk/ums_4.htm
 */


/*
 * USB Device Descriptor.
 */
static const uint8_t vcom_device_descriptor_data[18] = {
  USB_DESC_DEVICE       (0x0110,        /* bcdUSB USB version:
                                            0x0100 (USB1.0)
                                            0x0110 (USB1.1)
                                            0x0200 (USB2.0)
                                        */
                         0xFF,          /* bDeviceClass (FF=Vendor specific */
                         0x00,          /* bDeviceSubClass.                 */
                         0x00,          /* bDeviceProtocol.                 */
                         0x40,          /* bMaxPacketSize0.                 */
                         0x0483,        /* idVendor (ST).                   */
                         0xffff,        /* idProduct.                       */
                         0x0200,        /* bcdDevice Device Release Number  */
                         1,             /* Index of Manufacturer String Descriptor                  */
                         2,             /* iProduct.                        */
                         3,             /* iSerialNumber.                   */
                         1)             /* bNumConfigurations.              */
};

/*
 * Device Descriptor wrapper.
 */
static const USBDescriptor vcom_device_descriptor = {
  sizeof vcom_device_descriptor_data,
  vcom_device_descriptor_data
};

/*
 * Configuration Descriptor
 */
//MaxPacketsize for Bulk Full-Speed is 0x40
#define IN_PACKETSIZE  0x0040
#define OUT_PACKETSIZE 0x0040
#define EP_IN 1
#define EP_OUT 2
static const uint8_t vcom_configuration_descriptor_data[9+9+7+7] = {
  /* Configuration Descriptor.*/
  //9 Bytes
  USB_DESC_CONFIGURATION(sizeof vcom_configuration_descriptor_data,            /* wTotalLength.                    */
                         0x01,          /* bNumInterfaces.                  */
                         0x01,          /* bConfigurationValue.             */
                         0,             /* iConfiguration.                  */
                         0xC0,          /* bmAttributes (self powered).     */
                         50),           /* bMaxPower (100mA).               */
  /* Interface Descriptor.*/
  //9 Bytes
  USB_DESC_INTERFACE    (0x00,          /* bInterfaceNumber.                */
                         0x00,          /* bAlternateSetting.               */
                         0x02,          /* bNumEndpoints.                   */
                         0xFF,          /* bInterfaceClass (VendorSpecific) */
                         0x00,          /* bInterfaceSubClass               */
                         0x00,          /* bInterfaceProtocol               */
                         0),            /* iInterface.                      */
  /* Endpoint 1 Descriptor, Direction in*/
  //7 Bytes
  USB_DESC_ENDPOINT     (EP_IN|USB_RTYPE_DIR_DEV2HOST,  /* bEndpointAddress */
                         0x02,                      /* bmAttributes (Bulk)  */
                         IN_PACKETSIZE,             /* wMaxPacketSize       */
                         0x00),                     /* bInterval            */
  // Endpoint 2 Descriptor Direction out
  //7 Bytes
  USB_DESC_ENDPOINT     (EP_OUT|USB_RTYPE_DIR_HOST2DEV,  // bEndpointAddress
                         0x02,                      // bmAttributes (Bulk)
                         OUT_PACKETSIZE,            // wMaxPacketSize
                         0x00)                      // bInterval
};

/*
 * Configuration Descriptor wrapper.
 */
static const USBDescriptor vcom_configuration_descriptor = {
  sizeof vcom_configuration_descriptor_data,
  vcom_configuration_descriptor_data
};

/*
 * U.S. English language identifier.
 */
static const uint8_t stringLanguage[] = {
  USB_DESC_BYTE(4),                     /* bLength.                         */
  USB_DESC_BYTE(USB_DESCRIPTOR_STRING), /* bDescriptorType.                 */
  USB_DESC_WORD(0x0409)                 /* wLANGID (U.S. English).          */
};

/*
 * Vendor string.
 */
static const uint8_t stringVendor[] = {
  USB_DESC_BYTE(38),                    /* bLength.                         */
  USB_DESC_BYTE(USB_DESCRIPTOR_STRING), /* bDescriptorType.                 */
  'S', 0, 'T', 0, 'M', 0, 'i', 0, 'c', 0, 'r', 0, 'o', 0, 'e', 0,
  'l', 0, 'e', 0, 'c', 0, 't', 0, 'r', 0, 'o', 0, 'n', 0, 'i', 0,
  'c', 0, 's', 0
};

/*
 * Device Description string.
 */
static const uint8_t stringDeviceDescriptor[] = {
  USB_DESC_BYTE(48),                    /* bLength.                         */
  USB_DESC_BYTE(USB_DESCRIPTOR_STRING), /* bDescriptorType.                 */
  'C', 0, 'h', 0, 'i', 0, 'b', 0, 'i', 0, 'O', 0, 'S', 0, '/', 0,
  'C', 0, 'u', 0, 's', 0, 't', 0, 'o', 0, 'm', 0, ' ', 0, 'H', 0,
  'a', 0, 'r', 0, 'd', 0, 'w', 0, 'a', 0, 'r', 0, 'e', 0
};

/*
 * Serial Number string.
 */
static const uint8_t stringSerialNumber[] = {
  USB_DESC_BYTE(12),                    /* bLength .                        */
  USB_DESC_BYTE(USB_DESCRIPTOR_STRING), /* bDescriptorType.                 */
  '0' + CH_KERNEL_MAJOR, 0, '.', 0,
  '0' + CH_KERNEL_MINOR, 0, '.', 0,
  '0' + CH_KERNEL_PATCH, 0
};

/*
 * String not found string.
 */
static const uint8_t stringNotFound[] = {
  USB_DESC_BYTE(34),                    /* bLength .                        */
  USB_DESC_BYTE(USB_DESCRIPTOR_STRING), /* bDescriptorType.                 */
  'S', 0, 't', 0, 'r', 0, 'i', 0, 'n', 0, 'g', 0, ' ', 0, 'n', 0,
  'o', 0, 't', 0, ' ', 0, 'f', 0, 'o', 0, 'u', 0, 'n', 0, 'd', 0
};

/*
 * Strings wrappers array.
 */
static const USBDescriptor vcom_strings[] = {
  {sizeof stringLanguage,           stringLanguage},
  {sizeof stringVendor,             stringVendor},
  {sizeof stringDeviceDescriptor,   stringDeviceDescriptor},
  {sizeof stringSerialNumber,       stringSerialNumber},
  {sizeof stringNotFound,           stringNotFound}
};



#endif // USBDESCRIPTOR_H_INCLUDED
