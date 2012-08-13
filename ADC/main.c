
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "ch.h"
#include "hal.h"
#include "myADC.h"

#include "usbdescriptor.h"

uint8_t receiveBuf[OUT_PACKETSIZE];
uint8_t transferBuf[IN_PACKETSIZE];

USBDriver *  	usbp = &USBD1;

uint8_t transmitting =0;
static Thread *tp = NULL;
uint8_t initUSB=0;

/*
 * Initiate USB transfer thread
 */
static WORKING_AREA(waInitUsbTransfer, 128);
static msg_t initUsbTransfer(void *arg) {

  (void)arg;
  uint8_t i;
  chRegSetThreadName("initUsbTransfer");

  while (TRUE) {
    while((((p1+BUFFLEN-p2)%BUFFLEN)<IN_PACKETSIZE) && !overflow){
      chThdSleepMilliseconds(1);
    }

    for (i=0;i<IN_PACKETSIZE;i++){
      transferBuf[i]=((uint8_t*) data)[p2];
      if(p1==p2) transferBuf[i]= 0xff;
      if(overflow){
          transferBuf[i]= overflow;
          overflow=0;
      }
      p2 = (p2+1)%BUFFLEN;
    }
    while(transmitting){
      chThdSleepMilliseconds(1);
    }

    transmitting = 1;
    usbPrepareTransmit(usbp, EP_IN, transferBuf, IN_PACKETSIZE);

    chSysLock();
    usbStartTransmitI(usbp, EP_IN);
    chSysUnlock();

  }
  return 0;
}

/*
 * data Transmitted Callback
 */
void dataTransmitted(USBDriver *usbp, usbep_t ep){
    (void) usbp;
    (void) ep;
    transmitting=0;
    palTogglePad(GPIOD, GPIOD_LED3);


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
    //osp == ep2outstate
    USBOutEndpointState *osp = usbp->epc[ep]->out_state;
    if(osp->rxcnt){
        switch(receiveBuf[0]){
            case '1':
                palTogglePad(GPIOD, GPIOD_LED3);
                break;
            case '2':
                palTogglePad(GPIOD, GPIOD_LED4);
                break;
            case '3':
                palTogglePad(GPIOD, GPIOD_LED5);
                break;
            case '4':
                palTogglePad(GPIOD, GPIOD_LED6);
                break;

        }
    }

    /*
     * Initiate next receive
     */
    usbPrepareReceive(usbp, EP_OUT, receiveBuf, OUT_PACKETSIZE);

    chSysLockFromIsr();
    usbStartReceiveI(usbp, EP_OUT);
    chSysUnlockFromIsr();
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
    palTogglePad(GPIOD, GPIOD_LED6);
    return;
  case USB_EVENT_ADDRESS:
    return;
  case USB_EVENT_CONFIGURED:

    /* Enables the endpoints specified into the configuration.
       Note, this callback is invoked from an ISR so I-Class functions
       must be used.*/
    chSysLockFromIsr();
    usbInitEndpointI(usbp, 1, &ep1config);
    usbInitEndpointI(usbp, 2, &ep2config);
    chSysUnlockFromIsr();
    //allow the main thread to init the transfers
    initUSB =1;
    return;
  case USB_EVENT_SUSPEND:
    return;
  case USB_EVENT_WAKEUP:
    return;
  case USB_EVENT_STALLED:
    return;
  }
  palTogglePad(GPIOD, GPIOD_LED5);
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
    if (dindex < 4){
      return &vcom_strings[dindex];
      }
      else{
          return &vcom_strings[4];
      }
  }
  palTogglePad(GPIOD, GPIOD_LED4);
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

  //Start System
  halInit();
  chSysInit();


  //Start and Connect USB
  usbStart(usbp, &config);
  usbConnectBus(usbp);

  //init the ADC
  myADCinit();


  //main loop, inits the USB transfers when it is told to do so
  while (TRUE) {
      while(!initUSB){
        chThdSleepMilliseconds(100);
      }
    palTogglePad(GPIOD, GPIOD_LED6);
    /*
     * Starts first receiving transaction
     * all further transactions are initiated by the dataReceived callback
     */
    usbPrepareReceive(usbp, EP_OUT, receiveBuf, 64);
    chSysLock();
    usbStartReceiveI(usbp, EP_OUT);
    chSysUnlock();
    /*
     * Starts first transfer
     * all further transactions are initiated by the dataTransmitted callback
     */
    tp = chThdCreateStatic(waInitUsbTransfer, sizeof(waInitUsbTransfer), NORMALPRIO, initUsbTransfer, NULL);
    initUSB=0;
  }
}
