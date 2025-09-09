
#include <msp430.h>


/**
 * main.c
 */
int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;     // stop watchdog timer
    PM5CTL0 &= ~LOCKLPM5;         // Unlock ports from power manager

    P2DIR &= ~BIT1;               // Set pin P2.1 to be an input; energy band 4
    P2REN |=  BIT1;               // Enable internal pullup/pulldown resistor on P2.1
    P2OUT |=  BIT1;               // Pullup selected on P2.1

    P2DIR |=  BIT6;               // Set pin 2.6 to be an output; Yellow LED

    P2IES |=  BIT1;               // Make P2.1 interrupt happen on the falling edge
    P2IFG &= ~BIT1;               // Clear the P2.1 interrupt flag
    P2IE  |=  BIT1;               // Enable P2.1 interrupt

    //////////////
    P2DIR &= ~BIT2;               // Set pin P2.2 to be an input; energy band 3
    P2REN |=  BIT2;               // Enable internal pullup/pulldown resistor on P2.2
    P2OUT |=  BIT2;               // Pullup selected on P2.2

    P2DIR |=  BIT7;               // Set pin 2.7 to be an output; Green LED

    P2IES |=  BIT2;               // Make P2.w interrupt happen on the falling edge
    P2IFG &= ~BIT2;               // Clear the P2.2 interrupt flag
    P2IE  |=  BIT2;               // Enable P2.2 interrupt
    ///////////////
    P2DIR &= ~BIT3;               // Set pin P2.3 to be an input; energy band 2
    P2REN |=  BIT3;               // Enable internal pullup/pulldown resistor on P2.3
    P2OUT |=  BIT3;               // Pullup selected on P2.3 

    P3DIR |=  BIT6;               // Set pin 3.6 to be an output; Red LED

    P2IES |=  BIT3;               // Make P2.3 interrupt happen on the falling edge
    P2IFG &= ~BIT3;               // Clear the P2.3 interrupt flag
    P2IE  |=  BIT3;               // Enable P2.3 interrupt
    ////////////////
    P2DIR &= ~BIT4;               // Set pin P2.4 to be an input; energy band 1
    P2REN |=  BIT4;               // Enable internal pullup/pulldown resistor on P2.4
    P2OUT |=  BIT4;               // Pullup selected on P2.4

    P3DIR |=  BIT7;               // Set pin 3.7 to be an output; Blue LED

    P2IES |=  BIT4;               // Make P2.4 interrupt happen on the falling edge
    P2IFG &= ~BIT4;               // Clear the P2.4 interrupt flag
    P2IE  |=  BIT4;               // Enable P2.4 interrupt


    return 0;
}

#pragma vector=PORT1_VECTOR       // ISR for Port 1
__interrupt void ISRP1(void){
    if(P2IFG & BIT1){             // Output 1 caused the interrupt
        P1OUT &= ~BIT0;            // 0 0
        P2OUT &= ~BIT6;
    }
    else if(P2IFG & BIT2){        // Output 2 caused the interrupt
        P1OUT |= BIT0;            // 0 1
        P2OUT &= ~BIT7;
    }
    else if(P2IFG & BIT3){         // Output 3 caused the interrupt
        P1OUT &= ~BIT0;            // 1 0
        P3OUT |= BIT6;
    }
    else if(P2IFG & BIT4){        // Output 4 caused the interrupt
        P1OUT |= BIT0;            // 1 1
        P3OUT |= BIT7;
    }

    P2IFG &= ~BIT1;           // clear interrupt flag of P1.1
    P2IFG &= ~BIT2;           // clear interrupt flag of P1.2
    P2IFG &= ~BIT3;           // clear interrupt flag of P1.3
    P2IFG &= ~BIT4;           // clear interrupt flag of P1.4
}
