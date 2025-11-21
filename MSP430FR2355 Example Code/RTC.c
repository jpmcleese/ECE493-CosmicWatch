/* --COPYRIGHT--,BSD
 * Copyright (c) 2018, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * --/COPYRIGHT--*/
//***************************************************************************************
//  Blink the LED Demo - Software Toggle P1.0
//
//  Description; Toggle P1.0 inside of a software loop using DriverLib.
//  ACLK = n/a, MCLK = SMCLK = default DCO
//
//                MSP4302355
//             -----------------
//         /|\|              XIN|-
//          | |                 |
//          --|RST          XOUT|-
//            |                 |
//            |             P1.0|-->LED
//
//  E. Chen
//  Texas Instruments, Inc
//  May 2018
//  Built with Code Composer Studio v8
//***************************************************************************************

#include <driverlib.h>
#include <msp430.h>
#include "UART.h"

volatile unsigned short ms = 0;
volatile unsigned short RTCSEC = 0;
volatile unsigned short RTCMIN = 0;
volatile unsigned short RTCHOUR = 0;
volatile unsigned short RTCDAY = 0;

volatile char rtc_str[6];

void uint_to_string(unsigned int num, char* str) {
    int i = 0;
    int j;
    char temp;
    
    // Handle 0 case
    if (num == 0) {
        str[0] = '0';
        str[1] = '\0';
        return;
    }
    
    // Convert number to string (reversed)
    while (num > 0) {
        str[i++] = '0' + (num % 10);
        num /= 10;
    }
    str[i] = '\0';
    
    // Reverse the string
    for (j = 0; j < i/2; j++) {
        temp = str[j];
        str[j] = str[i-1-j];
        str[i-1-j] = temp;
    }
}


int main(void) {

    //UART1init();
    

    volatile uint32_t i;

    // Stop watchdog timer
    WDT_A_hold(WDT_A_BASE);

    // Set P1.0 to output direction
    GPIO_setAsOutputPin(
        GPIO_PORT_P1,
        GPIO_PIN0
        );

    // Disable the GPIO power-on default high-impedance mode
    // to activate previously configured port settings
    PMM_unlockLPM5();

    TB0CCR0=1000;      //How many ticks to toggle
    TB0CCTL0 = CCIE;
    TB0CTL = MC__UP | ID__1 | TBSSEL__SMCLK | TBCLR;

    __enable_interrupt();

/*
    TA1CCR0=1000;      //How many ticks to toggle
    TA1CCTL0  = CCIE;           //TA1.0 will interrupt (non-vectored)
    TA1CTL = MC__UP | ID__1 | TASSEL__SMCLK | TACLR;
    */
/*
    while(1)
    {
        UART1string("Hello World! \r\n");
        // Toggle P1.0 output
        GPIO_toggleOutputOnPin(
            GPIO_PORT_P1,
            GPIO_PIN0
            );

        // Delay
        for(i=10000; i>0; i--);
    }
*/

    while (1) {
        /*
        uint_to_string(RTCHOUR, rtc_str);
        UART1string(rtc_str);
        UART1string(":");
        uint_to_string(RTCMIN, rtc_str);
        UART1string(rtc_str);
        UART1string(":");
        uint_to_string(RTCSEC, rtc_str);
        UART1string(rtc_str);
        UART1string("\r\n");
        */
        __no_operation();
    }
}

#pragma vector=TIMER0_B0_VECTOR  //TA1.0 ISR (non-vectored)
__interrupt void T1A0ISR(void){
    TB0CCTL0 &= ~CCIFG;
    if (ms == 999) {
        ms = 0;      
        if (RTCSEC == 59) {
            //P1OUT ^= 0x01;
            RTCSEC = 0;
            if (RTCMIN == 59) {
                RTCMIN = 0;
                if (RTCHOUR == 23) {
                    RTCMIN = 0;
                    RTCDAY++;
                }
                else {
                    RTCHOUR++;
                }
            }
            else {
                RTCMIN++;
            }
        }
        else {
            RTCSEC++;
            //P1OUT ^= 0x01;
        }
    }
    else {
        ms++;
    }
}
