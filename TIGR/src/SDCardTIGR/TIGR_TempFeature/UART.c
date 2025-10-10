//  Developed by Dr. Tolga Soyata           3/14/2024
//  This file contains functions/tables related to the UART operation

// On-board     UART:   eUSCI Module 0    Channel A: UART, SPI   Channel B: I2C, SPI
//                                        UCA0RXD=P4.3   UCA0TXD=P4.2
// Back channel UART:   eUSCI Module 1    Channel A: UART, SPI   Channel B: I2C, SPI
//                                        UCA1RXD=P3.5   UCA1TXD=P3.4
//
// UCAxIFG     register that contains two flags:  UCTXIFG     UCRXIFG
//             UCRXIFG  receive flag. 0=no new data   1=new data arrived
//             UCTXIFG  receive flag. 0=busy          1=ready to transmit
// UCAxTXBUF   Transmit buffer  (writing to this starts TX, UCTXIFG goes to 0)
// UCAxRXBUF   Receive buffer   (reading this clears UCRXIFG)

#include <msp430.h>
//#include "UART.h"


void initClockTo8MHz(){
    // Startup clock system with max DCO setting ~8MHz
    CSCTL0_H = CSKEY >> 8;                    // Unlock clock registers
    CSCTL1 = DCOFSEL_3 | DCORSEL;             // Set DCO to 8MHz
    CSCTL2 = SELA__VLOCLK | SELS__DCOCLK | SELM__DCOCLK;
    CSCTL3 = DIVA__1 | DIVS__1 | DIVM__1;     // Set all dividers
    CSCTL0_H = 0;                             // Lock CS registers
}



void initClockTo16MHz(){
    // Configure one FRAM waitstate as required by the device datasheet for MCLK
    // operation beyond 8MHz _before_ configuring the clock system.
    FRCTL0 = FRCTLPW | NWAITS_1;

    // Clock System Setup
    CSCTL0_H = CSKEY_H;                     // Unlock CS registers
    CSCTL1 = DCOFSEL_0;                     // Set DCO to 1MHz
    // Set SMCLK = MCLK = DCO, ACLK = LFXTCLK (VLOCLK if unavailable)
    CSCTL2 = SELA__LFXTCLK | SELS__DCOCLK | SELM__DCOCLK;
    // Per Device Errata set divider to 4 before changing frequency to
    // prevent out of spec operation from overshoot transient
    CSCTL3 = DIVA__4 | DIVS__4 | DIVM__4;   // Set all corresponding clk sources to divide by 4 for errata
    CSCTL1 = DCOFSEL_4 | DCORSEL;           // Set DCO to 16MHz
    // Delay by ~10us to let DCO settle. 60 cycles = 20 cycles buffer + (10us / (1/4MHz)).
    __delay_cycles(300);
    CSCTL3 = DIVA__1 | DIVS__1 | DIVM__1;   // Set all dividers to 1 for 16MHz operation

    CSCTL4 &= ~LFXTOFF;
    do{
        CSCTL5 &= ~LFXTOFFG;                  // Clear XT1 fault flag
        SFRIFG1 &= ~OFIFG;
    }while (SFRIFG1&OFIFG);                   // Test oscillator fault flag

    CSCTL0_H = 0;                             // Lock CS registers
}


// On-board UART = eUSCI Module 0 Channel A. UCA0RXD=P4.3   UCA0TXD=P4.2
void UART0init(unsigned long BaudRate){
    P4SEL0 |=  (BIT3 | BIT2);                 //Configure pin functions:
    P4SEL1 &= ~(BIT3 | BIT2);                 //UCA0RXD=P4.3   UCA0TXD=P4.2
    // Configure PJ.5 PJ.4 for external crystal oscillator
    PJSEL0 |= BIT4 | BIT5;                    // For XT1

    PM5CTL0 &= ~LOCKLPM5;                     //Turn ON GPIO

    // These settings are from SLAU367P, Table 30-5
    switch(BaudRate){
        // 9600 baud settings.    UCOS16=1   UCBR=52   UCBRF=1   UCBRS=0x49
        case 9600:   initClockTo8MHz();
                     UCA0CTLW0 = UCSWRST;           // put eUSCI in SW reset to change baud rate
                     UCA0CTLW0 |= UCSSEL__SMCLK;    // BRCLK=8000000
                     UCA0BR0 = 52;
                     UCA0BR1 = 0x00;
                     UCA0MCTLW |= UCOS16 | UCBRF_1 | 0x4900;
        // 115,200 baud settings. UCOS16=1   UCBR=8   UCBRF=10   UCBRS=0xF7
        case 115200: initClockTo16MHz();
                     UCA0CTLW0 = UCSWRST;           // put eUSCI in SW reset to change baud rate
                     UCA0CTLW0 |= UCSSEL__SMCLK;    // BRCLK=16000000
                     UCA0BR0 = 8;
                     UCA0BR1 = 0x00;
                     UCA0MCTLW |= UCOS16 | UCBRF_10 | 0xF700;
                     break;
    }
    UCA0CTLW0 &= ~UCSWRST;          // take it off SW reset
}


void UART0send(unsigned char data){
    //wait for any ongoing transmission
    while(~(UCA0IFG & UCTXIFG));
    UCA0TXBUF=data;
}


unsigned char UART0receive(){
    //return ASAP if no data
    if(~(UCA0IFG & UCRXIFG)) return 0;
    else return UCA0RXBUF;
}


// Back channel UART = eUSCI Module 1 Channel A.     UCA1RXD=P3.5   UCA1TXD=P3.4
void UART1init(unsigned long BaudRate){
    P3SEL0 |=  (BIT5 | BIT4);                 //Configure pin functions:
    P3SEL1 &= ~(BIT5 | BIT4);                 //UCA1RXD=P3.5   UCA1TXD=P3.4
    // Configure PJ.5 PJ.4 for external crystal oscillator
    PJSEL0 |= BIT4 | BIT5;                    // For XT1

    PM5CTL0 &= ~LOCKLPM5;                     //Turn ON GPIO

    // These settings are from SLAU367P, Table 30-5
    switch(BaudRate){
        // 9600 baud settings.    UCOS16=1   UC1BR=52   UC1BRF=1   UC1BRS=0x49
        case 9600:   initClockTo8MHz();
                     UCA1CTLW0 = UCSWRST;           // put eUSCI in SW reset to change baud rate
                     UCA1CTLW0 |= UCSSEL__SMCLK;    // BRCLK=8000000
                     UCA1BR0 = 52;
                     UCA1BR1 = 0x00;
                     UCA1MCTLW |= UCOS16 | UCBRF_1 | 0x4900;
                     break;
        // 57600 baud settings.    UCOS16=1   UC1BR=8   UC1BRF=10   UC1BRS=0xF7
        case 57600:  initClockTo8MHz();
                     UCA1CTLW0 = UCSWRST;           // put eUSCI in SW reset to change baud rate
                     UCA1CTLW0 |= UCSSEL__SMCLK;    // BRCLK=8000000
                     UCA1BR0 = 8;
                     UCA1BR1 = 0x00;
                     UCA1MCTLW |= UCOS16 | UCBRF_10 | 0xF700;
                     break;
        // 115,200 baud settings. UCOS16=1   UCBR=8   UCBRF=10   UCBRS=0xF7
        case 115200: initClockTo16MHz();
                     UCA1CTLW0 = UCSWRST;           // put eUSCI in SW reset to change baud rate
                     UCA1CTLW0 |= UCSSEL__SMCLK;    // BRCLK=16000000
                     UCA1BR0 = 8;
                     UCA1BR1 = 0x00;
                     UCA1MCTLW |= UCOS16 | UCBRF_10 | 0xF700;
                     break;
    }
    UCA1CTLW0 &= ~UCSWRST;          // take it off SW reset
}


void UART1send(unsigned char data){
    //wait for any ongoing transmission
    while(!(UCA1IFG & UCTXIFG));
    UCA1TXBUF=data;
}


void UART1string(unsigned char str[]){
    unsigned char *nextChr=str;
    unsigned char c;
    while((c=*nextChr++) != 0){
        UART1send(c);
    }
}


unsigned char UART1receive(){
    //return ASAP if no data
    if(~(UCA1IFG & UCRXIFG)) return 0;
    else return UCA1RXBUF;
}







