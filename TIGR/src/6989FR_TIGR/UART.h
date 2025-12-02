//  Developed by Dr. Tolga Soyata           3/14/2024
//  This is the header file for UART.c


#ifndef UART_H_
#define UART_H_

#include "msp430.h"

extern void UART1init(unsigned long);
extern void UART1send(unsigned char);
extern unsigned char UART1receive();
extern void UART1init(unsigned long);
extern void UART1send(unsigned char);
extern void UART1string(unsigned char *);
extern unsigned char UART1receive();


#endif /* UART_H_ */
