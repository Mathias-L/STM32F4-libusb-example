#include <stdlib.h>

#include "ch.h"
#include "hal.h"
#include "chprintf.h"

#include "myADC.h"



/*
 * Global lock for the ADC.
 * I should probably use proper locking mechanisms provided by chibios!
 */
int running=0;

uint8_t  p1=0,p2=0;
uint8_t  overflow=0;
uint16_t data[BUFFLEN];
uint16_t vref[BUFFLEN];
uint16_t temp[BUFFLEN];


/*
 * Defines for continuous scan conversions
 */
#define ADC_GRP2_NUM_CHANNELS   10
#define ADC_GRP2_BUF_DEPTH      2048
static adcsample_t samples2[ADC_GRP2_NUM_CHANNELS * ADC_GRP2_BUF_DEPTH];


/*
 * Internal Reference Voltage, according to ST this is 1.21V typical
 * with -40°C<T<+105°C its Min: 1.18V, Typ 1.21V, Max: 1.24V
 */
#define VREFINT 121
/*
 * The measured Value is initialized to 2^16/3V*2.21V
 */
uint32_t VREFMeasured = 26433;


/*
 * Error callback, does nothing
 */
static void adcerrorcallback(ADCDriver *adcp, adcerror_t err) {

  (void)adcp;
  (void)err;
  if(running){
    data[p1++]=0;
    overflow++;
  }
}

/*
 * This callback is called everytime the buffer is filled or half-filled
 * A second ring buffer is used to store the averaged data.
 * I should use a third buffer to store a timestamp when the buffer was filled.
 * I hope I understood how the Conversion ring buffer works...
 */

static void adccallback(ADCDriver *adcp, adcsample_t *buffer, size_t n) {

  (void)adcp;
  (void)n;

  unsigned int i,j;
  uint32_t sum=0;
  uint32_t vrefSum=0;
  uint32_t tempSum=0;
  if(n != ADC_GRP2_BUF_DEPTH/2) overflow++;
  for(i=0;i<ADC_GRP2_BUF_DEPTH/2;i++){
    for (j=0;j<8;j++){
      sum+=buffer[i*ADC_GRP2_NUM_CHANNELS+j];
    }
    vrefSum +=buffer[i*ADC_GRP2_NUM_CHANNELS+8];
    tempSum +=buffer[i*ADC_GRP2_NUM_CHANNELS+9];
  }
  vref[p1] = vrefSum/(ADC_GRP2_BUF_DEPTH/4/8);
  temp[p1] = tempSum/(ADC_GRP2_BUF_DEPTH/4/8);
  data[p1] = sum/(ADC_GRP2_BUF_DEPTH/4);

  // Only propagate 1/4th of the measured value to average VREF further
  VREFMeasured = (VREFMeasured*3+vref[p1])>>2;
  ++p1;
  p1 = p1%BUFFLEN;
  if(p1==p2) ++overflow;
}


static const ADCConversionGroup adcgrpcfg2 = {
  TRUE,                     //circular buffer mode
  ADC_GRP2_NUM_CHANNELS,    //Number of the analog channels
  adccallback,              //Callback function
  adcerrorcallback,         //Error callback
  0,                        /* CR1 */
  ADC_CR2_SWSTART,          /* CR2 */
  ADC_SMPR1_SMP_AN12(ADC_SAMPLE_3) | ADC_SMPR1_SMP_AN11(ADC_SAMPLE_3) |
  ADC_SMPR1_SMP_SENSOR(ADC_SAMPLE_3) | ADC_SMPR1_SMP_VREF(ADC_SAMPLE_3),  //sample times ch10-18
  0,                                                                        //sample times ch0-9
  ADC_SQR1_NUM_CH(ADC_GRP2_NUM_CHANNELS),                                   //SQR1: Conversion group sequence 13...16 + sequence length
//  ADC_SQR2_SQ8_N(ADC_CHANNEL_IN11)   | ADC_SQR2_SQ7_N(ADC_CHANNEL_IN11),    //SQR2: Conversion group sequence 7...12
  ADC_SQR2_SQ10_N(ADC_CHANNEL_SENSOR) | ADC_SQR2_SQ9_N(ADC_CHANNEL_VREFINT) |
  ADC_SQR2_SQ8_N(ADC_CHANNEL_IN11)   | ADC_SQR2_SQ7_N(ADC_CHANNEL_IN11) ,
  ADC_SQR3_SQ6_N(ADC_CHANNEL_IN11)   | ADC_SQR3_SQ5_N(ADC_CHANNEL_IN11) |
  ADC_SQR3_SQ4_N(ADC_CHANNEL_IN11)   | ADC_SQR3_SQ3_N(ADC_CHANNEL_IN11) |
  ADC_SQR3_SQ2_N(ADC_CHANNEL_IN11)   | ADC_SQR3_SQ1_N(ADC_CHANNEL_IN11)     //SQR3: Conversion group sequence 1...6
};

/*
 * print the remainder of the ring buffer of a continuous conversion
 */
void cmd_measureRead(BaseSequentialStream *chp, int argc, char *argv[]) {

  (void)chp;
  (void)argc;
  (void)argv;
  while(p1!=p2){
    chprintf(chp, "%U:%U-%U-%U  ", p2, data[p2], vref[p2], temp[p2]);
    if (data[p2]==0){
      chprintf(chp, "\r\n Error!\r\n  ", p2, data[p2]);
    }
    p2 = (p2+1)%BUFFLEN;
  }
  chprintf(chp, "\r\n");
  if(overflow){
    chprintf(chp, "Overflow: %U  \r\n", overflow);
    overflow=0;
  }
}

void myADCinit(void){
  palSetGroupMode(GPIOC, PAL_PORT_BIT(1),
                  0, PAL_MODE_INPUT_ANALOG);
  adcStart(&ADCD1, NULL);
  //enable temperature sensor and Vref
  adcSTM32EnableTSVREFE();
  adcStartConversion(&ADCD1, &adcgrpcfg2, samples2, ADC_GRP2_BUF_DEPTH);

}
