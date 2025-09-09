#include <msp430.h> 


/**
 * main.c
 */
int main(void)
{
	WDTCTL = WDTPW | WDTHOLD;	// stop watchdog timer
	
	return 0;
}

#pragma vector=PORT1_VECTOR       // ISR for Port 1
__interrupt void ISRP1(void){
    if(P1IFG & BIT1){             // Output 1 caused the interrupt
        P1OUT &= ~BIT0;            // 0 0
        P9OUT &= ~BIT7;
    }
    else if(P1IFG & BIT2){        // Output 2 caused the interrupt
        P1OUT |= BIT0;            // 0 1
        P9OUT &= ~BIT7;
    }
    else if(P1IFG & BIT3){         // Output 3 caused the interrupt
        P1OUT &= ~BIT0;            // 1 0
        P9OUT |= BIT7;
    }
    else if(P1IFG & BIT4){        // Output 4 caused the interrupt
        P1OUT |= BIT0;            // 1 1
        P9OUT |= BIT7;
    }

    P1IFG &= ~BIT1;           // clear interrupt flag of P1.1
    P1IFG &= ~BIT2;           // clear interrupt flag of P1.2
    P1IFG &= ~BIT3;           // clear interrupt flag of P1.3
    P1IFG &= ~BIT4;           // clear interrupt flag of P1.4
}
