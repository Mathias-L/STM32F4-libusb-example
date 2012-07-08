
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "ch.h"
#include "hal.h"

#include "usbdescriptor.h"

int transmitting = 0;
int receiving = 0;


uint8_t receiveBuf[64];
uint8_t transferBuf[64];

USBDriver *  	usbp = &USBD1;
#define EP_IN 1
#define EP_OUT 2


/*
 * data Transmitted Callback
 */
void dataTransmitted(USBDriver *usbp, usbep_t ep){
    (void) usbp;
    (void) ep;
    palTogglePad(GPIOD, GPIOD_LED3);
    //unlock transmitter
    transmitting = 0;
}

/**
 * @brief   IN EP1 state.
 */
static USBInEndpointState ep1instate;

/**
 * @brief   EP1 initialization structure (IN only).
 */
static const USBEndpointConfig ep1config = {
  USB_EP_MODE_TYPE_BULK,    //Type and mode of the endpoint
  NULL,                     //Setup packet notification callback (NULL for non-control EPs)
  dataTransmitted,          //IN endpoint notification callback
  NULL,                     //OUT endpoint notification callback
  IN_PACKETSIZE,            //IN endpoint maximum packet size
  0x0000,                   //OUT endpoint maximum packet size
  &ep1instate,              //USBEndpointState associated to the IN endpoint
  NULL,                     //USBEndpointState associated to the OUTendpoint
  NULL                      //Pointer to a buffer for setup packets (NULL for non-control EPs)
};

/*
 * data Received Callback
 * it writes the received Data to the output buffer
 * to have an echo effect
 */
void dataReceived(USBDriver *usbp, usbep_t ep){
    (void) usbp;
    (void) ep;
    uint16_t i;

    palTogglePad(GPIOD, GPIOD_LED4);

    //osp == ep2outstate
    USBOutEndpointState *osp = usbp->epc[ep]->out_state;
    for (i=0;i<osp->rxcnt;i++){
        //maybe replace receiveBuf with osp->something
        transferBuf[i] = receiveBuf[i];
    }
    receiving = 0;

    /*
     * Initiate next receive
     */
    usbPrepareReceive(usbp, EP_OUT, receiveBuf, 64);

    chSysLockFromIsr();
    usbStartReceiveI(usbp, EP_OUT);
    chSysUnlockFromIsr();
    palTogglePad(GPIOD, GPIOD_LED4);

}

/**
 * @brief   OUT EP2 state.
 */
USBOutEndpointState ep2outstate;

/**
 * @brief   EP2 initialization structure (OUT only).
 */
static const USBEndpointConfig ep2config = {
  USB_EP_MODE_TYPE_BULK,
  NULL,
  NULL,
  dataReceived,
  0x0000,
  OUT_PACKETSIZE,
  NULL,
  &ep2outstate,
  NULL
};


/*
 * Handles the USB driver global events.
 */
static void usb_event(USBDriver *usbp, usbevent_t event) {
  (void) usbp;
  switch (event) {
  case USB_EVENT_RESET:
    return;
  case USB_EVENT_ADDRESS:
    return;
  case USB_EVENT_CONFIGURED:
    chSysLockFromIsr();

    /* Enables the endpoints specified into the configuration.
       Note, this callback is invoked from an ISR so I-Class functions
       must be used.*/
    usbInitEndpointI(usbp, 1, &ep1config);
    usbInitEndpointI(usbp, 2, &ep2config);

    chSysUnlockFromIsr();
    return;
  case USB_EVENT_SUSPEND:
    return;
  case USB_EVENT_WAKEUP:
    return;
  case USB_EVENT_STALLED:
    return;
  }
  return;
}

static const USBDescriptor *get_descriptor(USBDriver *usbp,
                                           uint8_t dtype,
                                           uint8_t dindex,
                                           uint16_t lang) {

  (void)usbp;
  (void)lang;

  switch (dtype) {
  case USB_DESCRIPTOR_DEVICE:
    return &vcom_device_descriptor;
  case USB_DESCRIPTOR_CONFIGURATION:
    return &vcom_configuration_descriptor;
  case USB_DESCRIPTOR_STRING:
    if (dindex < 4)
      return &vcom_strings[dindex];
  }
  return NULL;
}

  /**
   * Requests hook callback.
   * This hook allows to be notified of standard requests or to
   *          handle non standard requests.
   * This implementation does nothing and passes all requests to
   *     the upper layers
   */
bool_t requestsHook(USBDriver *usbp) {
    (void) usbp;
    return FALSE;
}

/**
 * USBconfig
 */
const USBConfig   	config =   {
    usb_event,
    get_descriptor,
    requestsHook,
    NULL
  };


int main(void) {
  transferBuf[0] =  'h';
  transferBuf[1] =  'e';
  transferBuf[2] =  'l';
  transferBuf[3] =  'l';
  transferBuf[4] =  'o';
  transferBuf[5] =  ' ';
  transferBuf[6] =  'U';
  transferBuf[7] =  'S';
  transferBuf[8] =  'B';
  transferBuf[9] =  '!';
  transferBuf[10] =  0;

  //Start System
  halInit();
  chSysInit();


  //Start and Connect USB
  usbStart(usbp, &config);
  usbConnectBus(usbp);

  //Some sleep needed?
  palTogglePad(GPIOD, GPIOD_LED6);
  chThdSleepMilliseconds(1000);
  palTogglePad(GPIOD, GPIOD_LED6);

  /*
   * Starts first receiving transaction
   * all further transactions are initiated by the dataReceived callback
   */
  usbPrepareReceive(usbp, EP_OUT, receiveBuf, 64);
  chSysLock();
  usbStartReceiveI(usbp, EP_OUT);
  chSysUnlock();


  while (TRUE) {
    //proper locking required
    while(transmitting){
      chThdSleepMilliseconds(1000);
    }
    transmitting = 1;
    //initiate transmission
    // should be replaced with something usefull, e.g. from an ADC_complete callback
    usbPrepareTransmit(usbp, EP_IN, transferBuf, sizeof transferBuf);

    chSysLock();
    usbStartTransmitI(usbp, EP_IN);
    chSysUnlock();

  }
}
