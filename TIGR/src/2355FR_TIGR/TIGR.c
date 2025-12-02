//  _______ _____ _____ _____   
// |__   __|_   _/ ____|  __ \ 
//    | |    | || |  __| |__) |
//    | |    | || | |_ |  _  / 
//    | |   _| || |__| | | \ \  
//    |_|  |_____\_____|_|  \_\ 
//
//  Tiny Instrument for Gathering Radiation
//  Version: 2.4-FR2355
//  Date: 10/14/2025
//
//  Authors:
//    - Kevin Nguyen
//    - Daniel Uribe
//    - Arda Cobanoglu
//    - JP McLeese III
//
//  Description:
//    This version of TIGR is adapted for MSP430FR2355 from the original
//    MSP430FR6989 implementation. Key changes include:
//    - Software RTC using Timer_B0 (FR2355 lacks hardware RTC)
//    - ADC peripheral differences (ADC vs ADC12)
//    - LED2 moved from P9.7 to P6.6
//
//  Changelog:
//    - Adapted for MSP430FR2355 microcontroller
//    - Implemented software RTC using Timer_B0
//    - Updated temperature sensor for FR2355 ADC
//    - SD card functionality maintained via eUSCI_B0 SPI
//


#include <string.h>
#include "tigr_config.h"
#include "tigr_mmc.h"
#include "sd_utils.h"
#include "tigr_utils.h"
#include "temp_utils.h"

// Global Variables - Definitions (declared extern in tigr_config.h)
EnergyReading readings[MAX_READINGS];
volatile unsigned int reading_count = 0;
volatile unsigned int muon_count = 0;

// SD Card variables
unsigned char sd_buffer[SD_BUFFER_SIZE];
unsigned long current_sector = 0;
unsigned int buffer_position = 0;
volatile unsigned char sd_initialized = 0;

// Software RTC variables (FR2355 doesn't have hardware RTC)
volatile unsigned int rtc_year = 0x2025;
volatile unsigned char rtc_month = 0x10;
volatile unsigned char rtc_day = 0x14;
volatile unsigned char rtc_hour = 0x12;
volatile unsigned char rtc_minute = 0x00;
volatile unsigned char rtc_second = 0x00;
volatile unsigned int rtc_ms = 0;

// Days in each month (index 0 unused, 1=Jan, etc.)
static const unsigned char days_in_month[] = {0, 31, 28, 29, 31, 30, 31, 30, 31, 31, 30, 31, 31};

// Helper function to check if year is leap year (BCD format)
static unsigned char is_leap_year(unsigned int year_bcd) {
    // Convert BCD to decimal
    unsigned int year = ((year_bcd >> 12) & 0xF) * 1000 +
                        ((year_bcd >> 8) & 0xF) * 100 +
                        ((year_bcd >> 4) & 0xF) * 10 +
                        (year_bcd & 0xF);
    return ((year % 4 == 0) && (year % 100 != 0)) || (year % 400 == 0);
}

// Helper to get max days in current month (BCD values)
static unsigned char get_max_days(unsigned char month_bcd, unsigned int year_bcd) {
    unsigned char month = ((month_bcd >> 4) & 0xF) * 10 + (month_bcd & 0xF);
    if (month == 2 && is_leap_year(year_bcd)) {
        return 29;
    }
    if (month >= 1 && month <= 12) {
        return days_in_month[month];
    }
    return 31;
}

// Increment BCD value with rollover
static unsigned char bcd_increment(unsigned char bcd, unsigned char max_val) {
    unsigned char low = bcd & 0x0F;
    unsigned char high = (bcd >> 4) & 0x0F;
    
    low++;
    if (low > 9) {
        low = 0;
        high++;
    }
    
    unsigned char result = (high << 4) | low;
    unsigned char decimal = high * 10 + low;
    
    if (decimal > max_val) {
        return (max_val == 59 || max_val == 23) ? 0x00 : 0x01;
    }
    return result;
}

// Increment BCD year
static unsigned int bcd_year_increment(unsigned int year_bcd) {
    unsigned int y = ((year_bcd >> 12) & 0xF) * 1000 +
                     ((year_bcd >> 8) & 0xF) * 100 +
                     ((year_bcd >> 4) & 0xF) * 10 +
                     (year_bcd & 0xF);
    y++;
    return ((y / 1000) << 12) | (((y / 100) % 10) << 8) | 
           (((y / 10) % 10) << 4) | (y % 10);
}

// Initialize software RTC using Timer_B0
void rtc_init(void) {
    // Configure Timer_B0 for 1ms interrupts
    // Using ACLK (32768 Hz) with divider
    // For 1 second: count to 32768
    // For 10ms: count to 328 (approximately)
    
    TB0CTL = TBSSEL__ACLK | MC__UP | TBCLR;  // ACLK, Up mode, clear timer
    TB0CCR0 = 327;                            // ~10ms period (32768/100 â‰ˆ 328)
    TB0CCTL0 = CCIE;                          // Enable CCR0 interrupt
}

// MSP430 and peripherals initialization
void msp_init(void) {
    WDTCTL = WDTPW | WDTHOLD;     // Stop watchdog timer
    PM5CTL0 &= ~LOCKLPM5;         // Unlock ports from power manager
    
    // LED configuration for FR2355 LaunchPad
    // LED1 = P1.0 (Red)
    // LED2 = P6.6 (Green)
    P1DIR |= BIT0;                // LED1 (P1.0) set as output
    P6DIR |= BIT6;                // LED2 (P6.6) set as output
    
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
    
    P2IES |=  BIT2;               // Make P2.2 interrupt happen on the falling edge
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
    
    // Software RTC Initialization (FR2355 doesn't have hardware RTC)
    // Set initial time values (already set as global variable defaults)
    rtc_year = 0x2025;            // Year (BCD)
    rtc_month = 0x10;             // Month (BCD)
    rtc_day = 0x14;               // Day (BCD)
    rtc_hour = 0x12;              // Hour (BCD)
    rtc_minute = 0x00;            // Minute (BCD)
    rtc_second = 0x00;            // Seconds (BCD)
    rtc_ms = 0;                   // Milliseconds counter
    
    // Initialize Timer_B0 for software RTC
    rtc_init();
    
    // Initialize ADC for temperature sensing
    adc_init();

    __enable_interrupt();         // Enable global interrupts
    
    P1OUT &= ~BIT0;               // Initialize LEDs to off
    P6OUT &= ~BIT6;
}

int main(void) {
    msp_init();
    
    // Small delay after init
    __delay_cycles(500000);
    
    // Initialize SD card
    sd_card_init();
    
    // Write CSV header to SD card buffer
    strcpy((char*)sd_buffer, "Muon#,Band,Date,Time,TempC\n");
    buffer_position = strlen((char*)sd_buffer);
    
    while(1) {
        __low_power_mode_3();
        __delay_cycles(500000);   // Delay by half a second
        P1OUT &= ~BIT0;           // Reset LEDs
    }
}

// Timer_B0 CCR0 ISR - Software RTC tick (every ~10ms)
#pragma vector=TIMER0_B0_VECTOR
__interrupt void Timer_B0_ISR(void) {
    rtc_ms += 10;  // Increment by 10ms
    
    if (rtc_ms >= 1000) {
        rtc_ms = 0;
        
        // Increment seconds (BCD)
        rtc_second = bcd_increment(rtc_second, 59);
        if (rtc_second == 0x00) {
            // Seconds rolled over, increment minutes
            rtc_minute = bcd_increment(rtc_minute, 59);
            if (rtc_minute == 0x00) {
                // Minutes rolled over, increment hours
                rtc_hour = bcd_increment(rtc_hour, 23);
                if (rtc_hour == 0x00) {
                    // Hours rolled over, increment day
                    unsigned char max_days = get_max_days(rtc_month, rtc_year);
                    rtc_day = bcd_increment(rtc_day, max_days);
                    if (rtc_day == 0x01) {
                        // Day rolled over, increment month
                        rtc_month = bcd_increment(rtc_month, 12);
                        if (rtc_month == 0x01) {
                            // Month rolled over, increment year
                            rtc_year = bcd_year_increment(rtc_year);
                        }
                    }
                }
            }
        }
    }
}

// ISR for Port 2 - Muon detection interrupt
#pragma vector=PORT2_VECTOR
__interrupt void ISRP2(void) {
    if(P2IFG & BIT1) {            // Energy band 4 caused the interrupt
        P1OUT |= BIT0;            // LED1 on
        P6OUT |= BIT6;            // LED2 on (P6.6 on FR2355)
        save_reading(4);
    }
    else if(P2IFG & BIT2) {       // Energy band 3 caused the interrupt
        P1OUT |= BIT0;            // LED1 on
        P6OUT &= ~BIT6;           // LED2 off
        save_reading(3);
    }
    else if(P2IFG & BIT3) {       // Energy band 2 caused the interrupt
        P1OUT &= ~BIT0;           // LED1 off
        P6OUT |= BIT6;            // LED2 on
        save_reading(2);
    }
    else if(P2IFG & BIT4) {       // Energy band 1 caused the interrupt
        P1OUT &= ~BIT0;           // LED1 off
        P6OUT &= ~BIT6;           // LED2 off
        save_reading(1);
    }
    muon_count++;
    if(reading_count >= MAX_READINGS){
        // Array is full - save to SD card and reset
        write_readings_to_sd();
        reading_count = 0;
    }
    
    P2IFG &= ~(BIT1 | BIT2 | BIT3 | BIT4);  // Clear interrupt flags
    __low_power_mode_off_on_exit();
}