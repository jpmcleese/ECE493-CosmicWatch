//  Developed by Dr. Tolga Soyata           3/14/2024
//  This is the header file for UART.c


#ifndef UART_H_
#define UART_H_

#include "msp430.h"

extern void UART1send(unsigned char);
extern void UART1init();
extern void UART1string(unsigned char *);


#endif /* UART_H_ */