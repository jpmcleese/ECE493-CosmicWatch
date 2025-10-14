//  _______ _____ _____ _____   
// |__   __|_   _/ ____|  __ \ 
//    | |    | || |  __| |__) |
//    | |    | || | |_ |  _  / 
//    | |   _| || |__| | | \ \  
//    |_|  |_____\_____|_|  \_\ 
//
//  Tiny Instrument for Gathering Radiation
//  Version: 2.4
//  Date: 10/14/2025
//
//  Authors:
//    - Kevin Nguyen
//    - Daniel Uribe
//    - Arda Cobanoglu
//    - JP McLeese III
//
//  Description:
//    This version of TIGR extends Version 2.4 with an onboard temperature sensor
//    using the MSP430’s internal ADC. A separate file was created for this feature,
//    since the ADC was initially avoided to minimize power consumption on
//    coin-cell-powered systems.
//
//  Changelog:
//    - Added UART debug messages showing what would be written to the SD card
//      (to be removed in the final release).
//
//    - Integrated MSP430 internal temperature sensor to log on-chip temperature (°C)
//      at each interrupt event.
//
//    - Note: SD card data is written in raw sector format — a hex editor
//      is required to view contents since no FAT filesystem is used
//      (excluded to save power and memory).
//
//    - SD Card functionality complete. Refer to tigr_mmc.h for wiring guide!
//
//  Next Steps: 
//    - Test SD card via a Coin battery powered MSP430 Launchpad to ensure program works
//      in a different envirorment. 
//    
//  Power Impact Note (From Frank):
//    - The MSP430 internal temperature sensor and ADC draw power only while active.
//    - During conversion, the ADC + reference consume ≈200–250 µA for ~1–2 ms.
//      → Sampling once per second ≈ 0.25 µA average current (minimal impact).
//      → Sampling once per minute or on rare events ≈ 0.004 µA (negligible).
//      → Continuous operation ≈ 250 µA (significant drain; not recommended).
//    - Best practice: enable ADC only when needed (e.g., at muon interrupts),
//      then power it down immediately after conversion. (Changes must be implemented to code)


#include <string.h>
#include "tigr_config.h"
#include "tigr_mmc.h"
#include "sd_utils.h"
#include "tigr_utils.h"
#include "temp_utils.h"
#include "UART.h"

// Global Variables - Definitions (declared extern in tigr_config.h)
EnergyReading readings[MAX_READINGS];
volatile unsigned int reading_count = 0;
volatile unsigned int muon_count = 0;

// SD Card variables
unsigned char sd_buffer[SD_BUFFER_SIZE];
unsigned long current_sector = 0;
unsigned int buffer_position = 0;
volatile unsigned char sd_initialized = 0;

// MSP430 and peripherals initialization
void msp_init(void) {
    WDTCTL = WDTPW | WDTHOLD;     // stop watchdog timer
    PM5CTL0 &= ~LOCKLPM5;         // Unlock ports from power manager
    
    // Initialize UART 
    UART1init(115200);
    __delay_cycles(200000);       // Let UART stabilize
    
    P9DIR |= BIT7;                // LED2 (P9.7) set as output
    P1DIR |= BIT0;                // LED1 (P1.0) set as output
    
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
    
    // RTC Initialization
    RTCCTL0_H = RTCKEY_H;                   // Unlock RTC
    RTCCTL1 = RTCBCD | RTCHOLD | RTCMODE;   // BCD mode, Calendar mode, Hold
    
    // Clear and set RTC values
    RTCYEAR = 0x2025;                       // Year 
    RTCMON = 0x10;                          // Month 
    RTCDAY = 0x14;                          // Day 
    RTCDOW = 0x02;                          // Day of week 
    RTCHOUR = 0x12;                         // Hour 
    RTCMIN = 0x00;                          // Minute 
    RTCSEC = 0x00;                          // Seconds 
    
    RTCCTL1 &= ~(RTCHOLD);                  // Start RTC
    RTCCTL0_H = 0;                          // Lock RTC
    
    // Initialize ADC for temperature sensing
    adc_init();

    UCA1IE |= UCRXIE;           // Enable USCI_A1 RX interrupt
    __enable_interrupt();       // Enable global interrupts
    
    P1OUT &= ~BIT0;             // Initialize LEDs to off
    P9OUT &= ~BIT7;
}

int main(void) {
    msp_init();
    
    // Small delay after init
    __delay_cycles(500000);
    
    // Send startup message
    UART1string("\r\n\r\n");
    UART1string("***************************************\r\n");
    UART1string("*    TIGR - Radiation Detector v2.4   *\r\n");
    UART1string("*                                     *\r\n");
    UART1string("***************************************\r\n");
    UART1string("System initializing...\r\n\r\n");
    
    // Display RTC status BEFORE SD init
    UART1string("=========== RTC Status Check ===========\r\n");
    char rtc_str[6];
    
    UART1string("Year  : 0x");
    hex_to_string_4(RTCYEAR, rtc_str);
    UART1string((unsigned char*)rtc_str);
    
    UART1string("\r\nMonth : 0x");
    bcd_to_string(RTCMON, rtc_str);
    UART1string((unsigned char*)rtc_str);

    UART1string("\r\nDay   : 0x");
    bcd_to_string(RTCDAY, rtc_str);
    UART1string((unsigned char*)rtc_str);
    
    UART1string("\r\nHour  : 0x");
    bcd_to_string(RTCHOUR, rtc_str);
    UART1string((unsigned char*)rtc_str);
    
    UART1string("\r\nMinute: 0x");
    bcd_to_string(RTCMIN, rtc_str);
    UART1string((unsigned char*)rtc_str);
    
    UART1string("\r\nSecond: 0x");
    bcd_to_string(RTCSEC, rtc_str);
    UART1string((unsigned char*)rtc_str);
    UART1string("\r\n========================================\r\n\r\n");
    
    // Test temperature sensor
    UART1string("======= Temperature Sensor Test ========\r\n");
    
    // Read and display raw ADC and calibration values
    while(!(REFCTL0 & REFGENRDY));  // Wait for reference
    ADC12CTL0 |= ADC12ENC | ADC12SC;
    while (ADC12CTL1 & ADC12BUSY);
    unsigned int raw_adc = ADC12MEM0;
    ADC12CTL0 &= ~ADC12ENC;
    
    unsigned int cal_30 = *((unsigned int *)0x1A1A);
    unsigned int cal_85 = *((unsigned int *)0x1A1C);
    
    UART1string("Raw ADC Value: ");
    char debug_val[12];
    uint_to_string(raw_adc, debug_val);
    UART1string((unsigned char*)debug_val);
    UART1string(" (should be ~2400-2600 at room temp)\r\n");  
    
    int current_temp = read_temperature();
    UART1string("Calculated Temperature: ");
    char temp_str[12];
    int_to_string(current_temp, temp_str);
    UART1string((unsigned char*)temp_str);
    UART1string(" C\r\n");
    UART1string("========================================\r\n\r\n");
    
    sd_card_init();  // Initialize SD card
    
    // Optional: Write header to SD card
    UART1string("Writing CSV header to buffer...\r\n");
    strcpy((char*)sd_buffer, "Muon#,Band,Date,Time,TempC\n");
    buffer_position = strlen((char*)sd_buffer);
    UART1string("Header prepared: ");
    UART1string(sd_buffer);
    
    if (!sd_initialized) {
        UART1string("\r\nRunning in DEBUG mode (no SD card)\r\n");
        UART1string("All data will be displayed on terminal\r\n");
    }
    
    UART1string("\r\nSystem ready! Waiting for muon detections...\r\n");
    
    while(1) {
        __low_power_mode_3();
        __delay_cycles(500000);   // Delay by half a second
        P1OUT &= ~BIT0;                   // reset LEDs
    }
}

// ISR for Port 2 - Muon detection interrupt
#pragma vector=PORT2_VECTOR
__interrupt void ISRP1(void) {
    UART1string("\r\n>>> Muon detected! ");
    
    if(P2IFG & BIT4) {            // Energy band 4 caused the interrupt
        P1OUT |= BIT0;            // 1 1
        P9OUT |= BIT7;
        save_reading(4);
    }
    else if(P2IFG & BIT3) {       // Energy band 3 caused the interrupt
        P1OUT |= BIT0;            // 1 0
        P9OUT &= ~BIT7;
        save_reading(3);
    }
    else if(P2IFG & BIT2) {       // Energy band 2 caused the interrupt
        P1OUT &= ~BIT0;           // 0 1
        P9OUT |= BIT7;
        save_reading(2);
    }
    else if(P2IFG & BIT1) {       // Energy band 1 caused the interrupt
        P1OUT &= ~BIT0;           // 0 0
        P9OUT &= ~BIT7;
        save_reading(1);
    }
    muon_count++;
    if(reading_count >= MAX_READINGS){
        // Array is full - save to SD card and reset
        UART1string("Buffer full, writing to SD...\r\n");
        write_readings_to_sd();
        reading_count = 0;
    }
    
    P2IFG &= ~(BIT1 | BIT2 | BIT3 | BIT4);  // clear interrupt flags
    __low_power_mode_off_on_exit();
}
