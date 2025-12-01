// UART.h
// UART interface for TIGR project
// Adapted for MSP430FR2355

#ifndef UART_H_
#define UART_H_

#include "msp430.h"

// UART1 functions (mapped to UCA0 on FR2355)
extern void UART1init(unsigned long BaudRate);
extern void UART1send(unsigned char data);
extern unsigned char UART1receive(void);

// UART1 functions (also mapped to UCA0 on FR2355 for compatibility)
// On FR2355 LaunchPad, backchannel UART uses UCA0 (P1.6/P1.7)
extern void UART1init(unsigned long BaudRate);
extern void UART1send(unsigned char data);
extern void UART1string(unsigned char *str);
extern unsigned char UART1receive(void);

#endif /* UART_H_ */
