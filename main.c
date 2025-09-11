// |__   __|_   _/ ____|  __ \
//    | |    | || |  __| |__) |
//    | |    | || | |_ |  _  /
//    | |   _| || |__| | | \ \
//    |_|  |_____\_____|_|  \_\
//
// Date: 9/11/2025
// Version: 1
// Authors: Kevin Nguyen, Daniel Uribe
//
// Description: 4 inputs where each pin will be connected to comparator. Falling edge interrupts
// with priority to higher energy band. Currently displays energy band to on board LEDs and does
// not save energy band onto RAM.
//

#include <msp430.h>

int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;     // stop watchdog timer
    PM5CTL0 &= ~LOCKLPM5;         // Unlock ports from power manager

    P9DIR   |= BIT7;             //LED2 (P9.7) set as output
    P1DIR   |= BIT0;             //LED1 (P1.0) set as output

    P2DIR &= ~BIT1;               // Set pin P2.1 to be an input; energy band 1
    P2REN |=  BIT1;               // Enable internal pullup/pulldown resistor on P2.1
    P2OUT |=  BIT1;               // Pullup selected on P2.1

    P2IES |=  BIT1;               // Make P2.1 interrupt happen on the falling edge
    P2IFG &= ~BIT1;               // Clear the P2.1 interrupt flag
    P2IE  |=  BIT1;               // Enable P2.1 interrupt

    //////////////
    P2DIR &= ~BIT2;               // Set pin P2.2 to be an input; energy band 2
    P2REN |=  BIT2;               // Enable internal pullup/pulldown resistor on P2.2
    P2OUT |=  BIT2;               // Pullup selected on P2.2

    P2IES |=  BIT2;               // Make P2.w interrupt happen on the falling edge
    P2IFG &= ~BIT2;               // Clear the P2.2 interrupt flag
    P2IE  |=  BIT2;               // Enable P2.2 interrupt
    ///////////////
    P2DIR &= ~BIT3;               // Set pin P2.3 to be an input; energy band 3
    P2REN |=  BIT3;               // Enable internal pullup/pulldown resistor on P2.3
    P2OUT |=  BIT3;               // Pullup selected on P2.3

    P2IES |=  BIT3;               // Make P2.3 interrupt happen on the falling edge
    P2IFG &= ~BIT3;               // Clear the P2.3 interrupt flag
    P2IE  |=  BIT3;               // Enable P2.3 interrupt
    ////////////////
    P2DIR &= ~BIT4;               // Set pin P2.4 to be an input; energy band 4
    P2REN |=  BIT4;               // Enable internal pullup/pulldown resistor on P2.4
    P2OUT |=  BIT4;               // Pullup selected on P2.4

    P2IES |=  BIT4;               // Make P2.4 interrupt happen on the falling edge
    P2IFG &= ~BIT4;               // Clear the P2.4 interrupt flag
    P2IE  |=  BIT4;               // Enable P2.4 interrupt

    __enable_interrupt();

    P1OUT &= ~BIT0;            // Initialize LEDs to off
    P9OUT &= ~BIT7;

    while(1){
        __low_power_mode_3();
    }
}

#pragma vector=PORT2_VECTOR       // ISR for Port 1
__interrupt void ISRP1(void){
    if(P2IFG & BIT4){             // Energy band 4 caused the interrupt
        P1OUT |= BIT0;            // 1 1
        P9OUT |= BIT7;
    }
    else if(P2IFG & BIT3){        // Energy band 3 caused the interrupt
        P1OUT |= BIT0;            // 1 0
        P9OUT &= ~BIT7;
    }
    else if(P2IFG & BIT2){         // Energy band 2 caused the interrupt
        P1OUT &= ~BIT0;            // 0 1
        P9OUT |= BIT7;
    }
    else if(P2IFG & BIT1){        // Energy band 1 caused the interrupt
        P1OUT &= ~BIT0;           // 0 0
        P9OUT &= ~BIT7;
    }

    P2IFG &= ~BIT1;           // clear interrupt flag of P1.1
    P2IFG &= ~BIT2;           // clear interrupt flag of P1.2
    P2IFG &= ~BIT3;           // clear interrupt flag of P1.3
    P2IFG &= ~BIT4;           // clear interrupt flag of P1.4
}
