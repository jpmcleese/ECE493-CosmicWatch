// |__   __|_   _/ ____|  __ \
//    | |    | || |  __| |__) |
//    | |    | || | |_ |  _  /
//    | |   _| || |__| | | \ \
//    |_|  |_____\_____|_|  \_\
//
// Tiny Intsrument For Gathering Radiation
// Date: 9/11/2025
// Version: 2.0
// Authors: Kevin Nguyen, Daniel Uribe, Arda Cobanoglu, JP Mcleese III
//
// Description: 4 inputs where each pin will be connected to comparator. Falling edge interrupts
// with priority to higher energy band. Currently displays energy band to on board LEDs and does
// not save energy band onto RAM.
//

#include <msp430.h>

// Structure to store energy band readings with timestamp
typedef struct {
    unsigned int muon_number;    // Muon event number
    unsigned char energy_band;   // Energy band (1-4)
    unsigned int year;           // Year
    unsigned char month;         // Month (1-12)
    unsigned char day;           // Day (1-31)
    unsigned char hour;          // Hour (0-23)
    unsigned char minute;        // Minute (0-59)
    unsigned char second;        // Second (0-59)
} EnergyReading;

// Array to store readings
#define MAX_READINGS 100                // Adjust as needed
EnergyReading readings[MAX_READINGS];
volatile unsigned int reading_count = 0;
volatile unsinged int muon_count = 0;

// Function to save current reading
void save_reading(unsigned char band) {
    if (reading_count < MAX_READINGS) {
        readings[reading_count].energy_band = band;
        readings[reading_count].muon_number = muon_count;

        // Read current RTC values (convert from BCD)
        readings[reading_count].year = RTCYEAR;
        readings[reading_count].month = RTCMON;
        readings[reading_count].day = RTCDAY;
        readings[reading_count].hour = RTCHOUR;
        readings[reading_count].minute = RTCMIN;
        readings[reading_count].second = RTCSEC;

        reading_count++;
    }
    else
    {
         /* save to sdcard and reset the array for more space 
            LOCAL SAVE CODE GOES HERE
        */
    }
    
    // Note: If array is full, oldest data gets lost.
}

void msp_init(){

    WDTCTL = WDTPW | WDTHOLD;     // stop watchdog timer
    PM5CTL0 &= ~LOCKLPM5;         // Unlock ports from power manager

    P9DIR |= BIT7;                //LED2 (P9.7) set as output
    P1DIR |= BIT0;                //LED1 (P1.0) set as output

    /*------ENERGY BAND 1-----*/
    P2DIR &= ~BIT1;               // Set pin P2.1 to be an input; energy band 1
    P2REN |=  BIT1;               // Enable internal pullup/pulldown resistor on P2.1
    P2OUT |=  BIT1;               // Pullup selected on P2.1

    P2IES |=  BIT1;               // Make P2.1 interrupt happen on the falling edge
    P2IFG &= ~BIT1;               // Clear the P2.1 interrupt flag
    P2IE  |=  BIT1;               // Enable P2.1 interrupt

    /*------ENERGY BAND 2-----*/
    P2DIR &= ~BIT2;               // Set pin P2.2 to be an input; energy band 2
    P2REN |=  BIT2;               // Enable internal pullup/pulldown resistor on P2.2
    P2OUT |=  BIT2;               // Pullup selected on P2.2

    P2IES |=  BIT2;               // Make P2.w interrupt happen on the falling edge
    P2IFG &= ~BIT2;               // Clear the P2.2 interrupt flag
    P2IE  |=  BIT2;               // Enable P2.2 interrupt

    /*------ENERGY BAND 3-----*/
    P2DIR &= ~BIT3;               // Set pin P2.3 to be an input; energy band 3
    P2REN |=  BIT3;               // Enable internal pullup/pulldown resistor on P2.3
    P2OUT |=  BIT3;               // Pullup selected on P2.3

    P2IES |=  BIT3;               // Make P2.3 interrupt happen on the falling edge
    P2IFG &= ~BIT3;               // Clear the P2.3 interrupt flag
    P2IE  |=  BIT3;               // Enable P2.3 interrupt

    /*------ENERGY BAND 4-----*/
    P2DIR &= ~BIT4;               // Set pin P2.4 to be an input; energy band 4
    P2REN |=  BIT4;               // Enable internal pullup/pulldown resistor on P2.4
    P2OUT |=  BIT4;               // Pullup selected on P2.4

    P2IES |=  BIT4;               // Make P2.4 interrupt happen on the falling edge
    P2IFG &= ~BIT4;               // Clear the P2.4 interrupt flag
    P2IE  |=  BIT4;               // Enable P2.4 interrupt

    RTCCTL1 = RTCBCD | RTCHOLD | RTCMODE;   // RTC enable, BCD mode, RTC hold

    RTCYEAR = 0x2024;                       // Year = 0x2024
    RTCMON = 0x4;                           // Month = 0x04 = April
    RTCDAY = 0x02;                          // Day = 0x02 = 2nd
    RTCDOW = 0x02;                          // Day of week = 0x02 = Tuesday
    RTCHOUR = 0x06;                         // Hour = 0x06
    RTCMIN = 0x32;                          // Minute = 0x32
    RTCSEC = 0x45;                          // Seconds = 0x45

    RTCCTL1 &= ~(RTCHOLD);                  // Start RTC

    __enable_interrupt();         // Enable global interrupts

    P1OUT &= ~BIT0;               // Initialize LEDs to off
    P9OUT &= ~BIT7;

}

int main(void)
{
    msp_init();
    while(1){
        __low_power_mode_3();
        __delay_cycles(500000);   // Delay by half a second
        P1OUT &= ~BIT0;           // reset LEDs
        P9OUT &= ~BIT7;
    }
}

#pragma vector=PORT2_VECTOR       // ISR for Port 2
__interrupt void ISRP1(void){
    if(P2IFG & BIT4){             // Energy band 4 caused the interrupt
        P1OUT |= BIT0;            // 1 1
        P9OUT |= BIT7;
        save_reading(4);
    }
    else if(P2IFG & BIT3){        // Energy band 3 caused the interrupt
        P1OUT |= BIT0;            // 1 0
        P9OUT &= ~BIT7;
        save_reading(3);
    }
    else if(P2IFG & BIT2){         // Energy band 2 caused the interrupt
        P1OUT &= ~BIT0;            // 0 1
        P9OUT |= BIT7;
        save_reading(2);
    }
    else if(P2IFG & BIT1){        // Energy band 1 caused the interrupt
        P1OUT &= ~BIT0;           // 0 0
        P9OUT &= ~BIT7;
        save_reading(1);
    }
    muon_count++;

    P2IFG &= ~(BIT1 | BIT2 | BIT3 | BIT4);  // clear interrupt flags for P2.1, P2.2, P2.3, P2.4
    __low_power_mode_off_on_exit();
}
