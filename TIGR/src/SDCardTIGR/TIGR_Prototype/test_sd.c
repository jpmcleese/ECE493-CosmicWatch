// test_sd.c
// Simple SD card test program for TIGR project
// Tests basic SD card read/write functionality

#include <msp430.h>
#include <string.h>
#include <stdio.h>
#include "tigr_mmc.h"

unsigned char test_buffer[512];
unsigned char read_buffer[512];

void test_delay(void) {
    __delay_cycles(1000000);  // ~1 second delay at 1MHz
}

int main(void) {
    unsigned char result;
    unsigned int i;
    unsigned char test_passed = 1;
    
    // Stop watchdog
    WDTCTL = WDTPW | WDTHOLD;
    PM5CTL0 &= ~LOCKLPM5;  // Unlock ports
    
    // Configure LEDs for status
    P1DIR |= BIT0;  // LED1 - Test running
    P9DIR |= BIT7;  // LED2 - Error indicator
    P1OUT &= ~BIT0;
    P9OUT &= ~BIT7;
    
    // Blink LED1 to indicate test start
    for (i = 0; i < 3; i++) {
        P1OUT |= BIT0;
        test_delay();
        P1OUT &= ~BIT0;
        test_delay();
    }
    
    // Check if card is present
    if (mmc_ping() != MMC_SUCCESS) {
        // No card detected - fast blink LED2
        while(1) {
            P9OUT ^= BIT7;
            __delay_cycles(200000);
        }
    }
    
    // Initialize SD card
    result = mmc_init();
    if (result != MMC_SUCCESS) {
        // Init failed - LED2 solid on
        P9OUT |= BIT7;
        while(1);
    }
    
    // Test 1: Write test pattern to sector 10
    // Fill buffer with test pattern
    for (i = 0; i < 512; i++) {
        test_buffer[i] = (unsigned char)(i & 0xFF);
    }
    
    P1OUT |= BIT0;  // LED1 on during write
    result = mmc_write_sector(10, test_buffer);
    P1OUT &= ~BIT0;
    
    if (result != MMC_SUCCESS) {
        P9OUT |= BIT7;  // Write failed
        test_passed = 0;
    }
    
    test_delay();
    
    // Test 2: Read back and verify
    memset(read_buffer, 0, 512);  // Clear read buffer
    
    P1OUT |= BIT0;  // LED1 on during read
    result = mmc_read_sector(10, read_buffer);
    P1OUT &= ~BIT0;
    
    if (result != MMC_SUCCESS) {
        P9OUT |= BIT7;  // Read failed
        test_passed = 0;
    }
    
    // Verify data
    for (i = 0; i < 512; i++) {
        if (read_buffer[i] != test_buffer[i]) {
            P9OUT |= BIT7;  // Data mismatch
            test_passed = 0;
            break;
        }
    }
    
    // Test 3: Write text message to sector 0
    memset(test_buffer, 0, 512);
    sprintf((char*)test_buffer, 
            "TIGR SD Card Test\n"
            "=================\n"
            "If you can read this, the SD card is working!\n"
            "MSP430FR6989 - TIGR Project\n"
            "Test completed successfully.\n");
    
    P1OUT |= BIT0;  // LED1 on during write
    result = mmc_write_sector(0, test_buffer);
    P1OUT &= ~BIT0;
    
    if (result != MMC_SUCCESS) {
        P9OUT |= BIT7;  // Write failed
        test_passed = 0;
    }
    
    // Test complete - show results
    if (test_passed) {
        // Success - slow blink LED1
        while(1) {
            P1OUT ^= BIT0;
            test_delay();
        }
    } else {
        // Failed - both LEDs on
        P1OUT |= BIT0;
        P9OUT |= BIT7;
        while(1);
    }
    
    return 0;
}