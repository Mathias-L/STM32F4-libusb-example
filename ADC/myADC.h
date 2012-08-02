#ifndef MYADC_H_INCLUDED
#define MYADC_H_INCLUDED
/*
 * second storage ring buffer for continuous scan
 */
#define BUFFLEN    1024
extern uint8_t  p1,p2;
extern uint8_t  overflow;
extern uint16_t data[BUFFLEN];
extern uint16_t vref[BUFFLEN];
extern uint16_t temp[BUFFLEN];

void myADCinit(void);


#endif // MYADC_H_INCLUDED
