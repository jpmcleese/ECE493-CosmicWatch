// |__   __|_   _/ ____|  __ \
//    | |    | || |  __| |__) |
//    | |    | || | |_ |  _  /
//    | |   _| || |__| | | \ \
//    |_|  |_____\_____|_|  \_\
//
// Tiny Instrument For Gathering Radiation
// Date: 9/11/2025
// Version: 2.1
// Authors: Kevin Nguyen, Daniel Uribe, Arda Cobanoglu, JP Mcleese III
//
// Description: 4 inputs where each pin will be connected to comparator. Falling edge interrupts
// with priority to higher energy band. Saves energy band data to SD card.
//

#include "tigr_config.h"
#include "tigr_mmc.h"

// Global Variables
EnergyReading readings[MAX_READINGS];
volatile unsigned int reading_count = 0;
volatile unsigned int muon_count = 0;

// SD Card variables
unsigned char sd_buffer[SD_BUFFER_SIZE];
unsigned long current_sector = 0;
unsigned int buffer_position = 0;
volatile unsigned char sd_initialized = 0;

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
    else {
        // Array is full - save to SD card and reset
        write_readings_to_sd();
        reading_count = 0;
    }
}

// Write readings to SD card
void write_readings_to_sd(void) {
    unsigned int i;
    char data_string[64];
    
    if (!sd_initialized) {
        // Try to initialize SD card if not already done
        sd_card_init();
        if (!sd_initialized) {
            // SD card not available - data will be lost
            // Turn on LED2 to indicate error
            P9OUT |= BIT7;
            return;
        }
    }
    
    // Format each reading and add to buffer
    for (i = 0; i < reading_count; i++) {
        // Format: "Muon#,Band,YYYY-MM-DD,HH:MM:SS\n"
        sprintf(data_string, "%u,%u,%04X-%02X-%02X,%02X:%02X:%02X\n",
                readings[i].muon_number,
                readings[i].energy_band,
                readings[i].year,
                readings[i].month,
                readings[i].day,
                readings[i].hour,
                readings[i].minute,
                readings[i].second);
        
        // Add to buffer
        unsigned int str_len = strlen(data_string);
        
        // Check if buffer would overflow
        if (buffer_position + str_len >= SD_BUFFER_SIZE) {
            // Write current buffer to SD card
            flush_buffer_to_sd();
        }
        
        // Copy string to buffer
        memcpy(&sd_buffer[buffer_position], data_string, str_len);
        buffer_position += str_len;
    }
    
    // Flush any remaining data
    if (buffer_position > 0) {
        flush_buffer_to_sd();
    }
}

// Flush buffer to SD card
void flush_buffer_to_sd(void) {
    if (buffer_position == 0) return;
    
    // Fill rest of buffer with zeros
    while (buffer_position < SD_BUFFER_SIZE) {
        sd_buffer[buffer_position++] = 0;
    }
    
    // Write buffer to SD card
    if (mmc_write_sector(current_sector, sd_buffer) == MMC_SUCCESS) {
        current_sector++;  // Move to next sector
        // Turn off error LED if write succeeded
        P9OUT &= ~BIT7;
    } else {
        // Write failed - indicate error
        P9OUT |= BIT7;  // Turn on LED2
    }
    
    // Reset buffer
    buffer_position = 0;
    memset(sd_buffer, 0, SD_BUFFER_SIZE);
}

// Initialize SD card
void sd_card_init(void) {
    unsigned char retry_count = 0;
    
    // Wait for card to be inserted
    while (!mmc_ping() && retry_count < 10) {
        __delay_cycles(1000000); // Wait 1 second
        retry_count++;
    }
    
    if (retry_count >= 10) {
        // No card detected
        sd_initialized = 0;
        return;
    }
    
    // Initialize the card
    retry_count = 0;
    while (mmc_init() != MMC_SUCCESS && retry_count < 3) {
        __delay_cycles(1000000); // Wait 1 second
        retry_count++;
    }
    
    if (retry_count >= 3) {
        sd_initialized = 0;
        P9OUT |= BIT7;  // Turn on LED2 to indicate SD error
    } else {
        sd_initialized = 1;
        P9OUT &= ~BIT7; // Turn off LED2
        // Clear the buffer
        memset(sd_buffer, 0, SD_BUFFER_SIZE);
    }
}

void msp_init(void) {
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
    
    RTCCTL1 = RTCBCD | RTCHOLD | RTCMODE;   // RTC enable, BCD mode, RTC hold
    
    RTCYEAR = 0x2025;                       // Year = 0x2025
    RTCMON = 0x09;                          // Month = 0x09 = September
    RTCDAY = 0x11;                          // Day = 0x11 = 11th
    RTCDOW = 0x04;                          // Day of week = 0x04 = Thursday
    RTCHOUR = 0x12;                         // Hour = 0x12
    RTCMIN = 0x00;                          // Minute = 0x00
    RTCSEC = 0x00;                          // Seconds = 0x00
    
    RTCCTL1 &= ~(RTCHOLD);                  // Start RTC
    
    __enable_interrupt();         // Enable global interrupts
    
    P1OUT &= ~BIT0;               // Initialize LEDs to off
    P9OUT &= ~BIT7;
}

int main(void) {
    msp_init();
    sd_card_init();  // Initialize SD card
    
    // Optional: Write header to SD card
    if (sd_initialized) {
        strcpy((char*)sd_buffer, "Muon#,Band,Date,Time\n");
        buffer_position = strlen((char*)sd_buffer);
    }
    
    while(1) {
        __low_power_mode_3();
        __delay_cycles(500000);   // Delay by half a second
        P1OUT &= ~BIT0;           // reset LEDs
        P9OUT &= ~BIT7;
    }
}

#pragma vector=PORT2_VECTOR       // ISR for Port 2
__interrupt void ISRP1(void) {
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
    
    P2IFG &= ~(BIT1 | BIT2 | BIT3 | BIT4);  // clear interrupt flags
    __low_power_mode_off_on_exit();
}